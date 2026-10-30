#!/usr/bin/env python3
"""KPlayer updater: explicit file allowlist, backups, and no shell execution."""
import hashlib
import io
import json
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import stat
import sys
import tempfile
import urllib.request
import zipfile

REPO = "kaiaum01/KPLAYER-v1.0.0"
API = "https://api.github.com/repos/" + REPO + "/releases/latest"
GAME = Path(__file__).resolve().parent
PORTS = GAME.parent
VERSION = GAME / "installed-version.txt"
ALLOWED = ("kplayer/kplayer", "kplayer/kplayer.gptk",
           "kplayer/lib/libvlc.so.5", "kplayer/lib/libvlccore.so.9",
           "kplayer/fonts/Interface.ttf", "kplayer/fonts/Bold.ttf")
MAX_DOWNLOAD = 32 * 1024 * 1024


class HTTPSOnly(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        if not newurl.startswith("https://"):
            raise ValueError("Redirecionamento inseguro")
        return super().redirect_request(req, fp, code, msg, headers, newurl)


def get(url, limit):
    if not url.startswith("https://"):
        raise ValueError("Endereço inseguro")
    req = urllib.request.Request(url, headers={"User-Agent": "KPlayer/1.0.0",
                                               "Accept": "application/vnd.github+json"})
    with urllib.request.build_opener(HTTPSOnly()).open(req, timeout=15) as response:
        data = response.read(limit + 1)
    if len(data) > limit:
        raise ValueError("Download excedeu o limite")
    return data


def version(value):
    match = re.fullmatch(r"v?(\d+)\.(\d+)\.(\d+)", value)
    if not match:
        raise ValueError("Versão inválida")
    return tuple(map(int, match.groups()))


def release_info(release, installed):
    tag = release.get("tag_name", "")
    if release.get("draft") or release.get("prerelease") or version(tag) <= version(installed):
        return None
    tag = "v" + ".".join(str(x) for x in version(tag))
    name = "KPlayer-" + tag + "-R36S.zip"
    assets = release.get("assets", [])
    if not isinstance(assets, list):
        raise ValueError("Lista de assets inválida")
    out = {}
    for suffix in ("", ".sha256"):
        matches = [a for a in assets if isinstance(a, dict) and a.get("name") == name + suffix]
        if len(matches) != 1:
            raise ValueError("Pacote ou checksum ausente")
        url = matches[0].get("browser_download_url", "")
        expected = "https://github.com/" + REPO + "/releases/download/" + tag + "/" + name + suffix
        if url != expected:
            raise ValueError("Endereço do pacote inesperado")
        out[suffix] = url
    return tag, out


def get_release(installed):
    return release_info(json.loads(get(API, 1024 * 1024)), installed)


def validate_archive(blob, checksum, tag):
    if not re.fullmatch(r"[0-9a-fA-F]{64}", checksum):
        raise ValueError("Checksum inválido")
    if hashlib.sha256(blob).hexdigest() != checksum.lower():
        raise ValueError("Checksum não confere")
    result = {}
    with zipfile.ZipFile(io.BytesIO(blob)) as z:
        entries = z.infolist()
        if len(entries) > 200 or sum(e.file_size for e in entries) > 64 * 1024 * 1024:
            raise ValueError("Pacote grande demais")
        seen = set()
        for entry in entries:
            name = entry.filename
            if (name.startswith("/") or "\\" in name or ".." in PurePosixPath(name).parts
                    or name in seen or stat.S_ISLNK(entry.external_attr >> 16)):
                raise ValueError("Caminho inseguro")
            seen.add(name)
            if name in ALLOWED:
                if entry.is_dir():
                    raise ValueError("Entrada inválida")
                result[name] = z.read(entry)
        if set(result) != set(ALLOWED) or z.testzip() is not None:
            raise ValueError("Pacote incompleto ou corrompido")
        try:
            manifest_info = z.getinfo("kplayer/update-manifest.json")
            if manifest_info.file_size > 4096:
                raise ValueError("Manifesto muito grande")
            manifest = json.loads(z.read(manifest_info))
        except (KeyError, json.JSONDecodeError) as exc:
            raise ValueError("Manifesto de atualização ausente ou inválido") from exc
        if (manifest.get("schema") != 1 or version(manifest.get("version", "")) != version(tag)
                or manifest.get("update_files") != list(ALLOWED)):
            raise ValueError("Manifesto incompatível")
    exe = result["kplayer/kplayer"]
    if len(exe) < 64 or exe[:6] != b"\x7fELF\x02\x01" or exe[18:20] != b"\xb7\x00":
        raise ValueError("Executável não é ARM64")
    return result


def install_files(files, tag, ports):
    # Only known application files; never touch user media, preferences or radios.
    ports = Path(ports)
    for name in ALLOWED:
        path = ports / name
        if any(part.is_symlink() for part in (path, path.parent, path.parent.parent)):
            raise ValueError("Caminho de destino contém link simbólico")
        if not path.is_file():
            raise ValueError("Instalação incompleta")
    backup = Path(tempfile.mkdtemp(prefix=".kplayer-backup-", dir=ports))
    staged = Path(tempfile.mkdtemp(prefix=".kplayer-stage-", dir=ports))
    replaced = []
    try:
        for name in ALLOWED:
            target = ports / name
            destination = staged / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_bytes(files[name])
            os.chmod(destination, target.stat().st_mode & 0o777)
            backup_file = backup / name
            backup_file.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(target, backup_file)
        for name in ALLOWED:
            os.replace(staged / name, ports / name)
            replaced.append(name)
        temp_version = staged / "installed-version.txt"
        temp_version.write_text(tag + "\n", encoding="ascii")
        os.replace(temp_version, ports / "kplayer" / "installed-version.txt")
    except Exception:
        for name in reversed(replaced):
            os.replace(backup / name, ports / name)
        raise
    finally:
        shutil.rmtree(staged, ignore_errors=True)
    return backup


def main():
    try:
        installed = VERSION.read_text(encoding="ascii").strip() if VERSION.exists() else "1.0.0"
        found = get_release(installed)
        if not found:
            print("LATEST")
            return 0
        tag, urls = found
        if len(sys.argv) == 1 or sys.argv[1] == "check":
            print("NEW " + tag)
            return 0
        if sys.argv[1] != "install" or len(sys.argv) != 3 or sys.argv[2] != tag:
            raise ValueError("A release mudou; verifique novamente")
        checksum_data = get(urls[".sha256"], 4096).decode("ascii").strip()
        expected_name = "KPlayer-" + tag + "-R36S.zip"
        match = re.fullmatch(r"([0-9a-fA-F]{64})\s+\*?([^\s]+)", checksum_data)
        if not match or match.group(2) != expected_name:
            raise ValueError("Checksum não corresponde ao pacote")
        payload = get(urls[""], MAX_DOWNLOAD)
        files = validate_archive(payload, match.group(1), tag)
        install_files(files, tag, PORTS)
        print("OK " + tag)
        return 0
    except Exception as exc:
        print("ERROR", file=sys.stdout)
        print(str(exc), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
