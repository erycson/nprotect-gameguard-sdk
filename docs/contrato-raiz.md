# Contrato do auxiliar da raiz

## Processo

- Alvo: `<cliente>\GameGuard.des`, executável PE x86.
- `GameGuard\GameGuard.des` é outro componente.
- O GME não deve ser alterado. Não exigir DLL auxiliar, launcher, Python ou chave privada no cliente.
- Não sinalizar o evento antes de verificar o INI e o GameMon; a sinalização do auxiliar não representa autenticação do servidor.

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

Validação de ponta a ponta da sequência de inicialização, da sincronização e da comunicação IPC com os clientes e o servidor.

## Argumento de inicialização do v584

O SDK v584 transforma o texto de inicialização em um quadro binário e o representa na linha de comando com cada byte no formato `\xNN`. O quadro contém um byte de formato (`1`), um byte usado como semente, dois bytes de comprimento e o conteúdo codificado. O algoritmo é uma transformação reversível, não uma assinatura nem uma garantia de integridade.

`gg_decode_launch_frame` recebe a representação `\xNN`, verifica o formato e o comprimento e devolve os bytes decodificados. Entradas truncadas, caracteres inválidos e conteúdo maior que o limite implementado são recusados. O conteúdo pode conter dados da sessão e não deve ser gravado em logs. No v584, a rotina de formatação usa um campo decimal, uma string e cinco campos hexadecimais. A v495 apresentou uma variação com quatro campos hexadecimais. A associação do PID e do evento dessa variação ainda precisa de validação em execução.

## Configuração observada

Na captura de memória do auxiliar associada ao cliente PH v500, a rotina `0x407100` verifica e lê o INI da identidade recebida. As chaves observadas na seção `GAMEMON` incluem `UPDATE_SERVER`, `UPDATE_PATH`, `BACKUP_SERVER`, `BACKUP_PATH`, `NO_UPDATE`, `NO_SPLASH`, `UP_SPLASH` e `NO_USE_SCAN`. A rotina compara a identidade configurada com a identidade informada pelo jogo; a captura isolada não prova que o GameMon aceite outro nome ou outra identidade. O caminho para padronização de um único INI precisa preservar essa associação e ser validado no GameMon.

## Sequência de inicialização observada na v584

O SDK no GME cria o `GameGuard.des` da raiz com herança de handles ativada e aguarda o evento passado ao auxiliar ou a finalização do processo. Após a etapa do auxiliar, o SDK inicia outro processo com herança de handles desativada e criação suspensa. A rotina de inicialização do SDK termina com `0x755` apenas após as demais etapas e a comunicação com o monitor.

A linha de comando do auxiliar pode conter somente o quadro `\xNN`, sem nome de executável precedendo-o. A entrada do programa deve considerar esse quadro também em `argv[0]`.

O conteúdo do INI é cifrado com RC4 por uma chave derivada de MD5 usando a CryptoAPI `PROV_RSA_FULL`; a verificação do rodapé e da assinatura deve ocorrer antes da descriptografia. `gg_decrypt_ini_payload` não verifica a assinatura por si só.

## Integridade do monitor

Na v584, `FUN_005c34a0` lê os 64 primeiros bytes da imagem do auxiliar em memória e inverte o DWORD em `0x400024`. O resultado é usado como CRC esperado do arquivo GameMon completo, inclusive assinatura e rodapé. O assinador preenche o campo do auxiliar com o complemento desse CRC antes de assiná-lo. Alterar o campo depois da assinatura invalida a assinatura do auxiliar.

`gg_validate_monitor` verifica o CRC completo e a assinatura identificada como `GameMon.des`; ela recebe a chave pública de confiança explicitamente. Isso não implementa o fluxo de inicialização do monitor.

## Evento do SDK

Após verificar o INI assinado, `GAME_NAME`, `NO_UPDATE=1`, a assinatura e o CRC do GameMon, o auxiliar sinaliza o evento herdado recebido no quadro de inicialização. O processo permanece vivo até trinta segundos, para permitir que o SDK leia o campo de CRC da imagem. A sinalização é uma etapa da inicialização e não indica que o GameMon iniciou ou que a autenticação foi aceita. A sequência completa ainda requer validação com o cliente real.

## Metadados de integridade

A implementação verifica as assinaturas e calcula os CRCs completos do INI selecionado, `GameMon.des`, `ggscan.des` e `npggNT.des`. Antes da sinalização, grava os quatro DWORDs em `HKCU\SOFTWARE\INCAInternet\nProtectGameGuard\Update` como `INF HashValue`, `DLL HashValue`, `DLL HashValue 2` e `DLL HashValue 3`. Ao encerrar a etapa, tenta restaurar os valores anteriores sem substituir um valor modificado por outro processo. O processo precisa de acesso de leitura e escrita a essa chave.

O cliente v584 iniciou o GameMon após a preparação dos quatro CRCs; a inicialização completa e a estabilidade permanecem por validar.

## Compatibilidade do monitor v584

O SDK v584 lê o auxiliar em `0x400000`; as builds usam essa base, sem ASLR nem relocação, e o assinador grava o CRC complementado no offset `0x24` antes da assinatura.

O GameMon usado nos testes tem SHA-256 do payload `4b8610a843274c942717036717b259fce999307a0c65e389050037403d154af9`. Para esse payload, o auxiliar instala em memória um manipulador restrito a quatro chamadas nulas identificadas, encaminhando-as às respectivas funções nativas do Windows. As demais exceções seguem o tratamento original. O arquivo GameMon não é alterado; outros payloads são recusados. A assinatura e o CRC são verificados separadamente.

O auxiliar verifica o PID do processo pai e remove, quando presente, o vínculo de depuração mantido pelo empacotador do cliente. Essa operação altera o estado de depuração do processo, sem alterar os bytes do GME. O detector lê a estrutura PE do arquivo e a seção `.text` do processo pai; uma seção parcialmente ilegível, um perfil desconhecido ou múltiplas correspondências são recusados.
