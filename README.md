# GameGuard interface

Projeto em C para o executÃ¡vel GameGuard.des da raiz, em Windows x86.

## Estrutura

- `modules/root-helper`: contrato e implementaÃ§Ã£o do auxiliar da raiz.
- `docs`: contratos de integraÃ§Ã£o e restriÃ§Ãµes.
- `scripts`: compilaÃ§Ã£o.
- `.dev`: testes e capturas locais, ignorados pelo Git.

## Estado da implementaÃ§Ã£o

O projeto contÃ©m um detector estrutural de SDK, parser do rodapÃ©, CRC32, verificaÃ§Ã£o RSA/MD5 via CryptoAPI e decodificaÃ§Ã£o do quadro de inicializaÃ§Ã£o v584. O ciclo de vida do auxiliar, a inicializaÃ§Ã£o do GameMon e a comunicaÃ§Ã£o IPC ainda nÃ£o estÃ£o implementados. O executÃ¡vel produzido Ã© um protÃ³tipo que falha de forma explÃ­cita e **nÃ£o Ã© um substituto funcional** para `GameGuard.des`.

Requer Windows, compilador MinGW-w64 i686 e Python 3 para o teste opcional de capturas. O binÃ¡rio do jogo e os arquivos GameGuard nÃ£o integram o repositÃ³rio.

```bat
compilar.bat
scripts\lint.cmd
.dev\test.cmd
```

`CC` pode apontar para `i686-w64-mingw32-gcc.exe`; sem `CC`, os scripts usam `gcc.exe` no PATH. O linter requer `clang-format` (ou `CLANG_FORMAT`) e GCC com `-fanalyzer`. A saÃ­da Ã© gravada em `build/`. Os testes nÃ£o executam imagens de clientes capturadas; `.dev/check-samples.py` lÃª somente suas seÃ§Ãµes `.text`.

## Variantes de compilação

`compilar.bat` oferece um menu para gerar CSAuth1, CSAuth2, híbrido ou todas as variantes. Também aceita `csauth1`, `csauth2`, `hybrid` ou `all` como argumento. As saídas ficam em `build/csauth1/GameGuard.des`, `build/csauth2/GameGuard.des` e `build/hybrid/GameGuard.des`.

O detector das variantes individuais rejeita a outra família; a híbrida aceita as duas. A detecção continua recusando resultados ambíguos. Estas opções não implementam o ciclo de vida ou a autenticação: todas as saídas continuam sendo protótipos que retornam erro.

## Chave pública

Defina `GG_PUBLIC_KEY` com o caminho de uma chave pública RSA de 512 bits em PEM antes de compilar. O script incorpora somente a chave pública ao executável; a chave privada não é utilizada na compilação. Sem essa variável, a compilação gera um protótipo sem chave que rejeita a inicialização. A versão compilada precisa receber o CRC e a assinatura por meio do assinador compatível antes de ser usada pelo SDK.
