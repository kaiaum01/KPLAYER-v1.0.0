# KPlayer R36S — compilação pelo GitHub Actions

Este pacote contém o código-fonte enviado e o workflow `.github/workflows/r36s-build.yml`. **O auto update ainda não foi implementado.** A compilação ARM64 e o aplicativo resultante ainda não foram testados no GitHub Actions nem no aparelho.

Para usar com um repositório já existente, faça uma cópia de segurança das suas alterações, clone o repositório usando o GitHub Desktop e copie somente `.github/workflows/r36s-build.yml` deste ZIP para a mesma pasta do clone. Não substitua a pasta `.git` nem os demais arquivos do seu repositório pelo ZIP. Faça commit e push do workflow. Na aba Actions, selecione **Compilar KPlayer R36S**, escolha **Run workflow** e, ao terminar com sucesso, baixe o artefato na página da execução.

O processo recompila o frontend e usa as bibliotecas libVLC já incluídas no projeto. O resultado é uma versão de teste, não uma Release publicada automaticamente. Se falhar, compartilhe o log da primeira etapa com erro.
