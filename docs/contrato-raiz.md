# Contrato do auxiliar da raiz

## Processo

- Alvo: `<cliente>\GameGuard.des`, executÃ¡vel PE x86.
- `GameGuard\GameGuard.des` Ã© outro componente.
- O GME nÃ£o deve ser alterado. NÃ£o exigir DLL auxiliar, launcher, Python ou chave privada no cliente.
- NÃ£o emitir prontidÃ£o ou sucesso de autenticaÃ§Ã£o sem confirmar o contrato real.

## Perfis do SDK

| Perfil estrutural | Objeto | Contexto | Comando local | Evento | AutenticaÃ§Ã£o |
|---|---:|---:|---:|---:|---|
| Legado PH | `0x1288` | `+0x105c` | `0x616` | `0x3f8` | CSAuth1 |
| Legado | `0x1288` | `+0x105c` | `0x619` | `0x3f9` | CSAuth2 |
| Novo | `0x2b98` | `+0x1468` | `0x619` | `0x3f9` | CSAuth2 |

SÃ£o trÃªs perfis de layout/comando e duas famÃ­lias de autenticaÃ§Ã£o; os rÃ³tulos nÃ£o sÃ£o nÃºmeros oficiais de SDK. A seleÃ§Ã£o no servidor permanece explÃ­cita. O comprimento do pacote isoladamente nÃ£o identifica a famÃ­lia.

Para `0x619`, o argumento observado Ã© um endereÃ§o para 16 bytes. Para `0x616`, Ã© um DWORD no quadro de comando. No canal de rede do jogo, ambos podem produzir um corpo de 16 bytes no `0x1016`, com tratamentos diferentes. NÃ£o presumir que todo IPC passe pelo auxiliar.

## Arquivos assinados

O rodapÃ© legado Ã© little-endian: payload, nome ASCII terminado em NUL, assinatura RSA de 64 bytes, seguido por quatro DWORDs: `0x32812622`, tamanho do nome, tamanho da assinatura e `0x32812621`. A assinatura Ã© RSA/MD5 sobre payload e nome. `gg_parse_footer` apenas valida estrutura; `gg_verify_signature` exige um `PUBLICKEYBLOB` explÃ­cito e executa a verificaÃ§Ã£o. A chave privada nÃ£o Ã© parte do projeto nem do cliente.

CRC32 usa polinÃ´mio refletido `0xedb88320`, inicializaÃ§Ã£o e finalizaÃ§Ã£o XOR `0xffffffff`. Os dados abrangidos por cada CRC devem ser definidos por cada consumidor.

## Pendente

Contrato completo de inicializaÃ§Ã£o, modo de lanÃ§amento e finalizaÃ§Ã£o do GameMon, sincronizaÃ§Ã£o, cÃ³digos de retorno, controle de integridade e encaminhamento IPC. O protÃ³tipo nÃ£o implementa nem simula esses comportamentos.

## Argumento de inicializaÃ§Ã£o do v584

O SDK v584 transforma o texto de inicializaÃ§Ã£o em um quadro binÃ¡rio e o representa na linha de comando com cada byte no formato `\xNN`. O quadro contÃ©m um byte de formato (`1`), um byte usado como semente, dois bytes de comprimento e o conteÃºdo codificado. O algoritmo Ã© uma transformaÃ§Ã£o reversÃ­vel, nÃ£o uma assinatura nem uma garantia de integridade.

`gg_decode_launch_frame` recebe a representaÃ§Ã£o `\xNN`, verifica o formato e o comprimento e devolve os bytes decodificados. Entradas truncadas, caracteres invÃ¡lidos e conteÃºdo maior que o limite implementado sÃ£o recusados. O conteÃºdo pode conter dados da sessÃ£o e nÃ£o deve ser gravado em logs. No v584, a rotina de formataÃ§Ã£o usa um campo decimal, uma string e cinco campos hexadecimais. A semÃ¢ntica de cada campo ainda precisa ser confirmada antes de ser usada na inicializaÃ§Ã£o do monitor.

## ConfiguraÃ§Ã£o observada

Na captura de memÃ³ria do auxiliar associada ao cliente PH v500, a rotina `0x407100` verifica e lÃª o INI da identidade recebida. As chaves observadas na seÃ§Ã£o `GAMEMON` incluem `UPDATE_SERVER`, `UPDATE_PATH`, `BACKUP_SERVER`, `BACKUP_PATH`, `NO_UPDATE`, `NO_SPLASH`, `UP_SPLASH` e `NO_USE_SCAN`. A rotina compara a identidade configurada com a identidade informada pelo jogo; a captura isolada nÃ£o prova que o GameMon aceite outro nome ou outra identidade. O caminho para padronizaÃ§Ã£o de um Ãºnico INI precisa preservar essa associaÃ§Ã£o e ser validado no GameMon.

## Sequência de inicialização observada na v584

O SDK no GME cria o `GameGuard.des` da raiz com herança de handles ativada e aguarda o evento passado ao auxiliar ou a finalização do processo. Após a etapa do auxiliar, o SDK inicia outro processo com herança de handles desativada e criação suspensa. A rotina de inicialização do SDK termina com `0x755` apenas após as demais etapas e a comunicação com o monitor.

A linha de comando do auxiliar pode conter somente o quadro `\xNN`, sem nome de executável precedendo-o. A entrada do programa deve considerar esse quadro também em `argv[0]`.

O conteúdo do INI é cifrado com RC4 por uma chave derivada de MD5 usando a CryptoAPI `PROV_RSA_FULL`; a verificação do rodapé e da assinatura deve ocorrer antes da descriptografia. `gg_decrypt_ini_payload` não verifica a assinatura por si só.
