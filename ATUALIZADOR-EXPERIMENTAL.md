# Atualizador experimental — R36S

A opção Atualização fica no menu de opções. B verifica a release final mais
recente em kaiaum01/KPLAYER-v1.0.0; se for mais recente e tiver o ZIP e o
checksum esperados, B inicia a atualização. Ao terminar, feche e reabra o app.
Rádio, músicas, configurações e preferências não fazem parte das substituições.

Requer Python 3 com módulos padrão, TLS e acesso HTTPS ao GitHub no R36S.
O pacote de atualização deve ter tag vX.Y.Z, arquivos KPlayer-vX.Y.Z-R36S.zip
e .zip.sha256 e manifesto compatível gerado pelo package_release.py. Para cada
versão futura o mantenedor deve ajustar VERSION, versão exibida no aplicativo e
os nomes/trigger dos workflows antes de publicar. Rascunhos e pré-releases não
são considerados pelo endpoint releases/latest.

Somente o executável, o arquivo gptk, as duas fontes e as duas bibliotecas
incluídas na lista de atualização são substituídos. O inicializador, o próprio
atualizador, arquivos novos e outros recursos não são atualizados: versões que
precisarem deles exigem instalação manual. Não publique uma release incompatível
como se pudesse ser atualizada por este mecanismo.

Em caso de exceção durante a substituição, tenta restaurar os arquivos alterados.
Backups ficam em .kplayer-backup-* dentro de ports; não são removidos
automaticamente. Faça backup próprio antes de testar. Falhas de energia não têm
recuperação automática. SHA-256 verifica integridade, mas não é uma assinatura;
a segurança também depende de HTTPS e da conta/repositório do mantenedor.

Testado localmente com arquivos simulados no Linux x86-64. O workflow ARM64 e
o funcionamento da atualização no R36S ainda precisam de testes reais; teste
primeiro em um cartão de cópia. Sem conexão ou sem release compatível, o menu
apresenta erro e o log tem detalhes. Não compartilhe dados privados do log.
