# GameGuard interface

Projeto em C para o executável GameGuard.des da raiz, em Windows x86.

## Estrutura

- `modules/root-helper`: contrato e implementação do auxiliar da raiz.
- `docs`: contratos de integração e restrições.
- `scripts`: compilação.
- `.dev`: testes e capturas locais, ignorados pelo Git.

## Estado da implementação

O projeto contém um detector estrutural de SDK, parser do rodapé, CRC32, verificação RSA/MD5 via CryptoAPI e decodificação do quadro de inicialização v584. A implementação experimental verifica o INI e o GameMon, valida o CRC e sinaliza o evento herdado no modo sem atualização. O SDK inicia o GameMon em processo separado. A compatibilidade de ponta a ponta ainda precisa de testes com os clientes e o servidor; **não há garantia de substituição funcional**.

Requer Windows, compilador MinGW-w64 i686 e Python 3 para o teste opcional de capturas. O binário do jogo e os arquivos GameGuard não integram o repositório.

```bat
compilar.bat
scripts\lint.cmd
.dev\test.cmd
```

`CC` pode apontar para `i686-w64-mingw32-gcc.exe`; sem `CC`, os scripts usam `gcc.exe` no PATH. O linter requer `clang-format` (ou `CLANG_FORMAT`) e GCC com `-fanalyzer`. A saída é gravada em `build/`. Os testes não executam imagens de clientes capturadas; `.dev/check-samples.py` lê somente suas seções `.text`.

## Variantes de compilação

`compilar.bat` oferece um menu para gerar CSAuth1, CSAuth2, híbrido ou todas as variantes. Também aceita `csauth1`, `csauth2`, `hybrid` ou `all` como argumento. As saídas ficam em `build/csauth1/GameGuard.des`, `build/csauth2/GameGuard.des` e `build/hybrid/GameGuard.des`.

O detector das variantes individuais rejeita a outra família; a híbrida aceita as duas. A detecção continua recusando resultados ambíguos. Estas opções selecionam a família aceita pelo detector; sua compatibilidade em execução ainda precisa ser validada.

## Chave pública

Defina `GG_PUBLIC_KEY` com o caminho de uma chave pública RSA de 512 bits em PEM antes de compilar. O script incorpora somente a chave pública ao executável; a chave privada não é utilizada na compilação. Sem essa variável, a compilação gera um protótipo sem chave que rejeita a inicialização. A versão compilada precisa receber o CRC e a assinatura por meio do assinador compatível antes de ser usada pelo SDK.
