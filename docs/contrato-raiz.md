# Contrato do auxiliar da raiz

## Processo

- Alvo: `<cliente>\GameGuard.des`, executável PE x86.
- `GameGuard\GameGuard.des` é outro componente.
- O GME não deve ser alterado. Não exigir DLL auxiliar, launcher, Python ou chave privada no cliente.
- Não emitir prontidão ou sucesso de autenticação sem confirmar o contrato real.

## Perfis do SDK

| Perfil estrutural | Objeto | Contexto | Comando local | Evento | Autenticação |
|---|---:|---:|---:|---:|---|
| Legado PH | `0x1288` | `+0x105c` | `0x616` | `0x3f8` | CSAuth1 |
| Legado | `0x1288` | `+0x105c` | `0x619` | `0x3f9` | CSAuth2 |
| Novo | `0x2b98` | `+0x1468` | `0x619` | `0x3f9` | CSAuth2 |

São três perfis de layout/comando e duas famílias de autenticação; os rótulos não são números oficiais de SDK. A seleção no servidor permanece explícita. O comprimento do pacote isoladamente não identifica a família.

Para `0x619`, o argumento observado é um endereço para 16 bytes. Para `0x616`, é um DWORD no quadro de comando. No canal de rede do jogo, ambos podem produzir um corpo de 16 bytes no `0x1016`, com tratamentos diferentes. Não presumir que todo IPC passe pelo auxiliar.

## Arquivos assinados

O rodapé legado é little-endian: payload, nome ASCII terminado em NUL, assinatura RSA de 64 bytes, seguido por quatro DWORDs: `0x32812622`, tamanho do nome, tamanho da assinatura e `0x32812621`. A assinatura é RSA/MD5 sobre payload e nome. `gg_parse_footer` apenas valida estrutura; `gg_verify_signature` exige um `PUBLICKEYBLOB` explícito e executa a verificação. A chave privada não é parte do projeto nem do cliente.

CRC32 usa polinômio refletido `0xedb88320`, inicialização e finalização XOR `0xffffffff`. Os dados abrangidos por cada CRC devem ser definidos por cada consumidor.

## Pendente

Contrato completo de inicialização, modo de lançamento e finalização do GameMon, sincronização, códigos de retorno, controle de integridade e encaminhamento IPC. O protótipo não implementa nem simula esses comportamentos.

## Argumento de inicialização do v584

O SDK v584 transforma o texto de inicialização em um quadro binário e o representa como hexadecimal na linha de comando. O quadro contém um byte de formato (`1`), um byte usado como semente, dois bytes de comprimento e o conteúdo codificado. O algoritmo é uma transformação reversível, não uma assinatura nem uma garantia de integridade.

`gg_decode_launch_frame` recebe a representação hexadecimal, verifica o formato e o comprimento e devolve os bytes decodificados. Entradas truncadas, caracteres inválidos e conteúdo maior que o limite implementado são recusados. O conteúdo pode conter dados da sessão e não deve ser gravado em logs. Os campos internos e sua semântica ainda precisam ser confirmados em uma execução real antes de serem usados na inicialização do monitor.
