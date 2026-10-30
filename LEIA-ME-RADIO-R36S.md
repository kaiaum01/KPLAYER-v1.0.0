# KPlayer — teste experimental do rádio no R36S

Este ZIP é o projeto para GitHub Desktop e Actions. Não é um pacote de
instalação recém-compilado. O binário ARM presente no código é da versão
anterior: gere outro pelo workflow `Compilar KPlayer R36S` antes de testar.

## Mudanças

- Ao mudar de categoria durante a busca, a resposta antiga é descartada e a
  categoria atual é buscada; a seleção permanece visível na lista.
- O endereço `favicon` da Radio Browser é usado como capa da emissora.
  O download ocorre fora da interface, limitado a 10 segundos e 2 MiB. É
  necessário ter o executável `curl` no sistema e a imagem ser JPEG ou PNG;
  quando isso não for possível, aparece o ícone padrão. Links inválidos,
  imagens ICO e outros formatos não são exibidos.
- A abertura e a troca do stream foram movidas para uma tarefa separada. Pode
  navegar e selecionar outra emissora enquanto uma troca está em andamento;
  a última seleção fica aguardando e será iniciada após a operação atual.
- A mensagem da rádio passa a indicar conexão enquanto o player não está no
  estado de reprodução.

## Limitações e testes

Se a libVLC ficar presa definitivamente na operação anterior, a nova rádio
continuará aguardando; também não é possível encerrar o aplicativo de forma
normal até essa operação retornar. Esta alteração não garante timeout nem
cancelamento da libVLC. O carregamento da lista ainda depende da rede e do DNS.

O código e os testes nativos foram compilados e executados em Linux x86-64;
a compilação ARM64 no GitHub Actions e o funcionamento no R36S/dArkOS ainda
não foram verificados. Não foi possível testar as rádios ou imagens reais
neste ambiente. Após compilar, teste várias emissoras, uma URL indisponível,
a troca rápida entre elas e o retorno à música local. Se houver erro, envie
o `log.txt` para ajudar no diagnóstico.
