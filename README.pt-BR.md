# KPlayer 1.0.0

Player de músicas, vídeos e rádios online para R36S, feito em C, SDL2 e libVLC.
Interface 640 × 480 para controles físicos no dArkOS / PortMaster.

[English](README.md) · [Instalação](docs/INSTALL.md) · [Licenças](THIRD_PARTY.md)

> **Candidata à versão final:** a correção do rádio foi relatada como funcional
> no R36S. Os temas, a barra de carregamento e a compilação ARM64 da 1.0.0 ainda
> precisam ser testados no aparelho. A atualização automática **não está
> implementada**. O código Android é experimental e não integra este lançamento.

## Prévias

Capturas da interface em 640 × 480, com dados demonstrativos, geradas no
computador; não são fotos do R36S.

| Tela inicial | Música |
| --- | --- |
| ![Dashboard do KPlayer](docs/images/v1-dashboard.png) | ![Player de música](docs/images/v1-audio.png) |
| **Rádio do Usuário** | **Configurações e temas** |
| ![Lista de rádios](docs/images/v1-radio.png) | ![Opções de tema](docs/images/v1-settings.png) |

## Funções

- Música e vídeo por libVLC; formatos dependem dos codecs e plugins do sistema.
- Busca em `midias`, `/roms` e `/roms2`, além de navegação manual.
- Metadados, capas JPEG/PNG, letras LRC e letras embutidas suportadas, fila e próxima faixa.
- Radio Browser e rádios do usuário em PLS, ASX, M3U e M3U8, com sete emissoras incluídas.
- Sete temas, PT-BR/inglês/espanhol, boas-vindas e termos de uso.
- Barra de carregamento sem porcentagem durante a busca.
- Vídeo em tela cheia, proporção, velocidade e seleção de faixas disponíveis.

## Instalação

Execute **Compilar KPlayer R36S** no GitHub Actions, baixe o artefato e extraia
`KPlayer-v1.0.0-R36S.zip`. Copie `KPlayer (Beta).sh` e `kplayer/` lado a lado
para a pasta `ports/` no cartão. O nome do inicializador foi mantido por
compatibilidade. Consulte [instalação](docs/INSTALL.md).
O ZIP do código-fonte não é o instalador.

Controles: B confirma, A volta, SELECT abre opções, START pausa e o direcional
navega. O volume usa os botões do aparelho. Veja [controles](docs/CONTROLS.md).
Coloque suas listas de rádio em `ports/kplayer/radios_usuario/` e pressione Y
na aba Rádio do Usuário para atualizar.

## Limitações

A busca automática ignora arquivos menores que 50 KiB e tem limite de 4.096
itens. Ela ocorre na tarefa da interface, então a barra pode parar se o cartão
bloquear. Alguns links ou variantes de playlists podem falhar. Logos de rádio
exigem `curl` opcional e imagens JPEG/PNG. Uma operação de rede da libVLC pode
ficar aguardando mesmo quando outra estação é selecionada.
Catálogo e localização aproximada usam serviços externos; consulte
[avisos de terceiros](THIRD_PARTY.md).

Antes de atualizar, faça cópia de `midias/`, `radios_usuario/`,
`settings.cfg`, `preferences.cfg` e `favorites.cfg`. Para relatar erros,
envie `ports/kplayer/log.txt` depois de remover informações pessoais.

Interface original: GPL-2.0-or-later; dependências têm licenças próprias.
Consulte [LICENSE](LICENSE) e [THIRD_PARTY.md](THIRD_PARTY.md).
Projeto sem afiliação oficial com VideoLAN ou PortMaster.
