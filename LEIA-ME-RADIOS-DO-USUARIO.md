# Rádios do usuário — R36S / PortMaster

Após gerar e instalar o pacote pelo GitHub Actions, copie arquivos para
`ports/kplayer/radios_usuario/` no cartão. Abra Rádio Online, use esquerda ou
direita para chegar a **Rádio do Usuário** e use Y para atualizar a lista.

O aplicativo procura endereços HTTP e HTTPS em arquivos PLS, ASX, M3U e
M3U8 de até 64 KiB. Cada endereço encontrado gera uma entrada com o nome do
arquivo; endereços seguintes recebem um número. Pastas dentro dessa pasta,
arquivos locais e outros formatos não são importados. Não há suporte confirmado
para AX3. Arquivos grandes, fluxos indisponíveis ou formatos de lista mais
complexos podem não funcionar. O exemplo da Rádio Ilha veio do PLS enviado
pelo usuário. Incluídas também Rádio Massa, Aline FM, Web Rádio Capital da
Amizade, Amizade FM, Abdala FM e Ilustrada FM 102.3. As transmissões não foram
testadas no aparelho. O arquivo da Rádio Massa mantém o endereço fornecido.

Ao atualizar uma instalação existente, preserve seus arquivos pessoais da pasta
radios_usuario; faça uma cópia de segurança e não substitua os de mesmo nome
sem conferir.

O código foi compilado e os testes locais para PLS, ASX e M3U passaram no
Linux x86-64. O binário ARM incluído no código-fonte é anterior às alterações:
gere um novo usando o workflow. O Actions e o R36S não foram testados aqui.
A limitação anterior sobre conexão da libVLC que pode não terminar permanece;
consulte LEIA-ME-RADIO-R36S.md para detalhes.
