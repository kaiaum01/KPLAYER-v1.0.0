# Publicar KPlayer 1.0.0 no GitHub

Este pacote é a árvore de código-fonte, sem o executável ARM antigo. A versão
1.0.0 deve ser tratada como candidata até compilar e testar no R36S. O
atualizador automático não está implementado.

1. Extraia o ZIP. No GitHub Desktop, crie um repositório local novo para o
   KPlayer, sem colocar a pasta extraída inteira dentro de outra pasta do
   projeto. Copie o **conteúdo** desta pasta para a raiz do repositório.
2. Confira se a raiz contém README.md, source/, ports/ e .github/. A pasta
   .github pode ficar oculta no Windows; ela é necessária para o Actions.
3. No GitHub Desktop revise os arquivos e publique seu repositório. Não envie
   mídias pessoais, logs, dados de configuração, senhas ou chaves.
4. No GitHub, abra Actions → Compilar KPlayer R36S e execute o workflow.
   Se necessário, habilite os workflows. Aguarde terminar com sucesso.
5. Baixe o artifact KPlayer-R36S e extraia-o. Ele contém
   KPlayer-v1.0.0-R36S.zip e o checksum .sha256. O artefato é temporário.
6. Teste o ZIP instalável no R36S; faça backup dos arquivos pessoais antes.
   Teste temas, idiomas, áudio, vídeo, busca, rádio e controles.
7. Depois de validado, crie uma Release com tag v1.0.0 no commit testado e
   anexe manualmente o ZIP instalável e o checksum. O workflow atual não faz
   a publicação automática. Descreva as limitações conhecidas na release.

GitHub Desktop precisa de um repositório Git real criado/clonado no computador:
um ZIP extraído sozinho não possui o histórico .git. A versão Android e o
auto-update não fazem parte da versão final R36S.
