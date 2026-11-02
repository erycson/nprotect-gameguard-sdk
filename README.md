# GameGuard interface

Projeto em C para o executável GameGuard.des da raiz, em Windows x86.

## Estrutura

- `modules/root-helper`: contrato e implementação do auxiliar da raiz.
- `docs`: contratos de integração e restrições.
- `scripts`: compilação.
- `.dev`: testes e capturas locais, ignorados pelo Git.

## Estado da implementação

O projeto contém um detector estrutural de SDK, parser do rodapé, CRC32, verificação RSA/MD5 via CryptoAPI e decodificação do quadro de inicialização v584. O ciclo de vida do auxiliar, a inicialização do GameMon e a comunicação IPC ainda não estão implementados. O executável produzido é um protótipo que falha de forma explícita e **não é um substituto funcional** para `GameGuard.des`.

Requer Windows, compilador MinGW-w64 i686 e Python 3 para o teste opcional de capturas. O binário do jogo e os arquivos GameGuard não integram o repositório.

```bat
scripts\build.cmd
scripts\lint.cmd
.dev\test.cmd
```

`CC` pode apontar para `i686-w64-mingw32-gcc.exe`; sem `CC`, os scripts usam `gcc.exe` no PATH. O linter requer `clang-format` (ou `CLANG_FORMAT`) e GCC com `-fanalyzer`. A saída é gravada em `build/`. Os testes não executam imagens de clientes capturadas; `.dev/check-samples.py` lê somente suas seções `.text`.
