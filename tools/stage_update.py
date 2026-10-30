"""Validation prototype only: never writes to the KPlayer installation."""
import hashlib
import hmac
import io
import re
import stat
import zipfile
from pathlib import PurePosixPath

MAX_ZIP = 32 * 1024 * 1024
MAX_UNPACKED = 64 * 1024 * 1024
MAX_ENTRIES = 200


def validate_archive(blob, expected_hash):
    if len(blob) > MAX_ZIP:
        raise ValueError("Pacote muito grande")
    if not re.fullmatch(r"[a-fA-F0-9]{64}", expected_hash):
        raise ValueError("Checksum inválido")
    if not hmac.compare_digest(hashlib.sha256(blob).hexdigest(), expected_hash.lower()):
        raise ValueError("Checksum não confere")
    with zipfile.ZipFile(io.BytesIO(blob)) as archive:
        entries = archive.infolist()
        if len(entries) > MAX_ENTRIES:
            raise ValueError("Entradas demais")
        seen = set()
        total = 0
        for item in entries:
            name = item.filename
            path = PurePosixPath(name)
            if (not name or name.startswith("/") or "\\" in name or
                    ".." in path.parts or name in seen):
                raise ValueError("Caminho inválido ou duplicado")
            seen.add(name)
            mode = item.external_attr >> 16
            if stat.S_ISLNK(mode):
                raise ValueError("Links simbólicos não são aceitos")
            total += item.file_size
            if total > MAX_UNPACKED:
                raise ValueError("Conteúdo descompactado muito grande")
        try:
            info = archive.getinfo("kplayer/kplayer")
        except KeyError as exc:
            raise ValueError("Executável ausente") from exc
        if info.is_dir() or info.file_size < 64 or info.file_size > MAX_ZIP:
            raise ValueError("Executável inválido")
        data = archive.read(info)
        if data[:6] != b"\x7fELF\x02\x01" or data[18:20] != b"\xb7\x00":
            raise ValueError("Executável não é ELF ARM64 little-endian")
        if archive.testzip() is not None:
            raise ValueError("Arquivo corrompido")
        return data
