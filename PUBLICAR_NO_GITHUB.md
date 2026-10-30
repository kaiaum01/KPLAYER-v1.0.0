# Publicar KPlayer (Beta) pelo GitHub Desktop

Este projeto já contém uma pasta Git inicializada, com branch `main`, sem commits, conta, e-mail de autor ou endereço remoto configurado. Nenhuma publicação foi realizada.

## 1. Adicionar o projeto

1. Extraia o ZIP completamente. Não abra os arquivos dentro da janela do ZIP.
2. Abra o GitHub Desktop e entre na sua conta.
3. Use **File → Add local repository → Choose**.
4. Selecione a pasta **KPlayer**, a mesma que contém `README.md`, `source` e `ports`.
5. Clique em **Add repository**.

Também é possível arrastar a pasta KPlayer para a janela do Desktop.

Se o programa disser que não é um repositório Git, confira se selecionou a pasta correta e se a extração preservou a pasta `.git`. Como alternativa, abra um terminal na pasta KPlayer e execute `git init -b main`; depois adicione essa mesma pasta ao Desktop. Não crie uma segunda pasta KPlayer dentro dela.

## 2. Fazer o primeiro commit

Na aba Changes, mantenha os arquivos do projeto selecionados. Em Summary, use:

`KPlayer (Beta) v0.7.1 — Players de áudio e rádio renovados`

Clique em **Commit to main**. O autor do commit será a identidade configurada na sua conta do GitHub Desktop.

## 3. Publicar

Clique em **Publish repository**. Sugestão:

- **Name:** `KPlayer`
- **Description:** `Native audio and video player for R36S / dArkOS / PortMaster. C, SDL2 and libVLC. PT-BR, English and Spanish. Beta.`
- Desmarque **Keep this code private** se quiser disponibilizar publicamente.
- Publique na sua conta pessoal e clique em **Publish repository**.

O nome apresentado no README é **KPlayer (Beta)**. O nome curto `KPlayer` é o sugerido para o endereço do repositório.

## 4. Criar a primeira release

Depois do upload, abra o repositório no navegador:

1. Acesse **Releases → Draft a new release**.
2. Crie a tag **`v0.7.1-beta`**, apontando para o commit publicado na branch `main`.
3. Use o título **`KPlayer (Beta) — v0.7.1`**.
4. Copie o texto de `releases/RELEASE_NOTES.md` para a descrição.
5. Anexe `releases/KPlayer-Beta-v0.7.1-R36S.zip` e seu arquivo `.sha256`.
6. Marque a versão como **pre-release** e publique quando estiver pronto.

O ZIP instalável já está gerado. Não anexe o ZIP externo do projeto como se fosse o pacote de instalação.

## O que está preparado

README em inglês e português com imagens; instalação, controles, compilação, mídias, changelog, créditos e licenças; fontes C e testes; motor VLC original e patch; arquivos compilados em `ports/`; ZIP instalável e notas da release.

O diretório `upstream/` inclui o tarball VLC necessário para os fontes correspondentes do motor. Use o GitHub Desktop para publicar a pasta completa. Os arquivos foram organizados sem músicas pessoais, logs, configurações de usuário, compiladores ou pastas temporárias de desenvolvimento.

## Documentação oficial

- https://docs.github.com/en/desktop/adding-and-cloning-repositories/adding-a-repository-from-your-local-computer-to-github-desktop
- https://docs.github.com/en/desktop/adding-and-cloning-repositories/adding-an-existing-project-to-github-using-github-desktop
- https://docs.github.com/en/repositories/releasing-projects-on-github/managing-releases-in-a-repository
