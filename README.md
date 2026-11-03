# GameGuard Interface

Implementação independente, em C++17, da interface `GameGuard.des` usada na integração entre clientes Windows x86 e o GameMon legado. O projeto fornece detecção das interfaces CSAuth1 e CSAuth2 e inicialização local sem atualização online.

**Descrição para o repositório:** An open-source Windows x86 implementation of the legacy GameGuard.des integration interface, with CSAuth1 and CSAuth2 support.

## Escopo e compatibilidade

- Implementa o executável `GameGuard.des` da raiz do cliente. O GameMon e os demais componentes GameGuard são fornecidos separadamente.
- Verifica assinaturas e CRC dos arquivos locais, lê o INI assinado e criptografado e sinaliza o SDK do cliente após a inicialização.
- Oferece variantes CSAuth1, CSAuth2 e híbrida. A híbrida detecta a família do SDK; a detecção rejeita resultados ambíguos.
- O modo atual exige `NO_UPDATE=1` e não implementa o serviço de atualização.
- A compatibilidade em memória do monitor é limitada ao GameMon v584 identificado no contrato técnico. O GME e o GameMon em disco não são modificados.
- A build híbrida C++ foi testada com o cliente v584 e o servidor em modo `required`, incluindo respostas sucessivas aceitas. Outros clientes e ambientes precisam de validação própria; não há garantia de compatibilidade universal nem de proteção equivalente ao produto original.

O projeto não inclui executáveis do jogo, GameMon, demais arquivos GameGuard nem chaves privadas. Esses componentes e suas respectivas licenças são independentes deste código-fonte.

## Compilação

Requisitos: Windows, PowerShell 7, MinGW-w64 i686 com suporte a C++17 e uma chave pública RSA de 512 bits em formato PEM.

```bat
set "CXX=C:\caminho\para\i686-w64-mingw32-g++.exe"
set "GG_PUBLIC_KEY=C:\caminho\para\chave_publica.pem"
compilar.bat all
```

`compilar.bat` também oferece um menu ou aceita `csauth1`, `csauth2` e `hybrid`. As saídas são gravadas em `build/<variante>/GameGuard.des`. O compilador usa ligação estática para o runtime C++ e `-s` para remover símbolos; o executável continua dependendo do UCRT do sistema.

A compilação incorpora somente a chave pública. Sem `GG_PUBLIC_KEY`, o protótipo resultante rejeita a inicialização. Antes do uso, o executável precisa receber o CRC do GameMon final e ser assinado com a chave privada correspondente, por uma ferramenta compatível. Não distribua a chave privada.

## Verificação e estrutura

`scripts\lint.cmd` executa as verificações de formatação e análise; requer `clang-format` e g++ com suporte a `-fanalyzer`. Os testes locais ficam em `.dev`, ignorada pelo Git; podem depender de arquivos e ferramentas disponíveis apenas no ambiente de desenvolvimento.

- `modules/root-helper`: implementação e contratos da interface.
- `docs`: documentação técnica e contratos de integração.
- `scripts`: scripts de compilação e verificação.

## Licença

Este código-fonte é distribuído sob a **GNU Affero General Public License, versão 3** (SPDX: `AGPL-3.0-only`). O texto completo está em [LICENSE](LICENSE).
