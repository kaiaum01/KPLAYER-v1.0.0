# KPlayer para Android (Beta 0.1.1)

Projeto Android nativo do KPlayer, separado da compilação dArkOS/PortMaster. A interface se adapta automaticamente a celulares e tablets em **retrato e paisagem**.

## Recursos desta primeira beta

- Primeira execução com PT-BR, inglês e espanhol, além dos Termos de uso.
- Biblioteca de áudio e vídeo do Android por MediaStore.
- Escolha manual de pastas pelo seletor seguro do sistema, com acesso persistente.
- Player de áudio com título, artista, álbum, formato, tamanho, duração e próxima faixa.
- Reprodução de vídeo e streams pelo libVLC para Android.
- Rádio Online usando o catálogo Radio Browser, com lista ao lado do player em paisagem e abaixo dele em retrato.
- Tema escuro KPlayer, fontes maiores e navegação por toque.

## Compilar automaticamente pelo GitHub Actions

O arquivo `.github/workflows/android-build.yml` gera o APK automaticamente.

1. Envie todo o conteúdo desta pasta para um repositório do GitHub.
2. Abra a aba **Actions** do repositório.
3. Selecione **Compilar KPlayer Android**.
4. Clique em **Run workflow** e confirme.
5. Quando a execução terminar, abra-a e baixe `KPlayer-Android-Beta-NÚMERO` em **Artifacts**.
6. Extraia o artefato para obter `KPlayer-Android-Beta.apk`.

O workflow também executa em novos envios para `main` ou `master`. Ao criar uma tag iniciada por `v`, como `v0.1.1-beta`, o APK será anexado automaticamente à Release correspondente.

## Compilar localmente no Windows

1. Instale o Android Studio atual.
2. Abra diretamente a pasta `android`.
3. Instale o Android SDK 36 quando solicitado e use o JDK 17 incorporado ao Android Studio.
4. Aguarde o Gradle baixar as dependências.
5. Conecte um aparelho com depuração USB e use **Run**, ou escolha **Build > Build APK(s)**.
6. O APK de depuração será criado em `app/build/outputs/apk/debug/app-debug.apk`.

O primeiro build requer internet para obter AndroidX e `org.videolan.android:libvlc-all:3.7.7`. Não compartilhe `local.properties`, `.gradle`, `.idea` ou pastas `build`.

## Permissões e pastas

No Android 13 ou superior, o KPlayer solicita acesso separado a músicas e vídeos. Em versões anteriores, solicita leitura de armazenamento. A opção **Escolher pasta** abre o seletor oficial do Android e mantém a autorização para aquela pasta. O aplicativo não depende de caminhos Linux como `/roms` e `/roms2`.

## Estado da beta

O código-fonte foi revisado estruturalmente. O APK será compilado pelos servidores do GitHub na primeira execução do workflow e depois deverá ser validado em um aparelho real. Serviço em segundo plano, controles da tela bloqueada e Android Auto ficam para uma versão posterior.
