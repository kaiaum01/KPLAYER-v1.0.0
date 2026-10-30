# Indicação de carregamento — KPlayer R36S

A tela de carregamento aparece ao procurar músicas, vídeos e ao abrir pastas
locais. A barra é indeterminada: ela não representa porcentagem. A contagem
indica quantos itens foram encontrados até o momento. O texto segue o idioma
escolhido no aplicativo.

A busca continua na mesma thread da interface: a barra é atualizada entre as
leituras de arquivos. Uma operação do sistema de arquivos que fique bloqueada
pode parar a animação temporariamente; esta mudança não corrige travamentos
reais nem garante que o usuário possa interromper a busca.

Projeto preparado para o GitHub Actions. O executável ARM já presente neste
projeto é anterior à alteração. Compile uma nova versão no Actions antes de
instalar. Os testes locais de compilação e nativos passaram em Linux x86-64,
mas a compilação ARM64 e o teste no R36S não foram realizados neste ambiente.
Ao atualizar, preserve os arquivos pessoais e playlists de rádio.
