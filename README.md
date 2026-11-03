# rt_log

`rt_log` oferece uma API de logs para aplicações Zephyr, com mensagens por
severidade e tag, filtros configuráveis e controle de nível em execução.
Use para registrar falhas, estado e informações de desenvolvimento de forma
uniforme entre os componentes da aplicação, ajustando o detalhamento por tag.

As macros `RT_LOGE`, `RT_LOGW`, `RT_LOGI`, `RT_LOGD` e `RT_LOGV` encaminham as
mensagens ao logger nativo do Zephyr. Formatação, timestamps, cores e saída
aproveitam a infraestrutura nativa; backend e modo de processamento são
configurados pela aplicação. Não é um coletor de métricas nem armazenamento
durável de eventos.

A API é inspirada na ESP-IDF, com implementação original Apache-2.0, independente
de nRF, ESP32, Seeed e Meshtastic. Não oferece substituição integral/ABI da
biblioteca ESP-IDF e não copia sua implementação.

## Uso

```c
#include <rt_log/log.h>

RT_LOG_MODULE_REGISTER(my_component); /* uma vez em cada .c/.cpp */

void example(double temperature)
{
    RT_LOGE("SENSOR", "falha=%d", -1);
    RT_LOGW("SENSOR", "tentando novamente");
    RT_LOGI("SENSOR", "temperatura=%02.04f", temperature);
    RT_LOGD("SENSOR", "estado=%u", 2u);
    RT_LOGV("SENSOR", "detalhe");
    RT_LOG_SCI("SENSOR", "corrente", 0.00156); /* corrente=1.56e-03 */

    rt_log_level_set("SENSOR", RT_LOG_WARN);
    rt_log_level_set("*", RT_LOG_INFO); /* limpa overrides e redefine padrão */
}
```

Formatos devem ser literais (ou macros de literais), seguindo a API básica estilo
ESP-IDF V1. Argumentos são avaliados somente se nível global, local e filtro de
tag permitem a mensagem; tag é avaliada uma vez quando compilada. Defina
RT_LOG_LOCAL_LEVEL antes do include para reduzir o teto de um arquivo, por exemplo
RT_LOG_WARN. O teto local não pode restaurar código removido pelo teto global.

Níveis: NONE=0, ERROR=1, WARN=2, INFO=3, DEBUG=4, VERBOSE=5. VERBOSE é distinto no
filtro da camada e aparece com marcador [V], sobre a severidade DBG nativa.
Com cores: erro vermelho, warning amarelo, info verde, debug/verbose azul.
O prefixo/timestamp continua no formato Zephyr, não é idêntico ao ESP-IDF.

## Kconfig do módulo

| Opção | Equivalência/conteúdo |
|---|---|
| RT_LOG | Habilitar a camada; exige LOG e CBPRINTF_COMPLETE |
| RT_LOG_DEFAULT_LEVEL | Nível inicial, como LOG_DEFAULT_LEVEL da ESP-IDF |
| RT_LOG_MAX_LEVEL | Teto compilado, como LOG_MAXIMUM_LEVEL |
| RT_LOG_DYNAMIC_LEVEL_CONTROL | Filtros mutáveis por tag, como LOG_DYNAMIC_LEVEL_CONTROL |
| RT_LOG_COLOR | Padrão de cores ANSI do backend nativo |
| RT_LOG_TIMESTAMP | Padrão de exibição do timestamp |
| RT_LOG_FORMAT_TIMESTAMP | Timestamp legível; desabilitar usa o formato bruto nativo |
| RT_LOG_SHOW_LEVEL | Exibir severidade no prefixo nativo |
| RT_LOG_TAG_SLOTS / TAG_MAX_LEN | Tabela fixa de overrides e limite dos nomes |
| RT_LOG_FLOAT_SUPPORT | Formatter nativo com %f, %e, %g etc. |

Os controles de apresentação fornecem defaults para LOG_BACKEND_SHOW_TIMESTAMP,
LOG_BACKEND_FORMAT_TIMESTAMP, LOG_BACKEND_SHOW_LEVEL, LOG_BACKEND_SHOW_COLOR,
LOG_INFO_COLOR_GREEN e LOG_DBG_COLOR_BLUE. Uma escolha explícita dessas opções
nativas prevalece. Formatos adicionais de tempo, backend UART/RTT e suas filas
continuam opções nativas do Zephyr; não se cria outro backend ou parser printf.

Comparação conferida com components/log/Kconfig{,.level,.format,.settings} da
instalação ESP-IDF do usuário. ESP-IDF também oferece V1/V2, texto/binário,
tempo de sistema/Unix e opções de IRAM/bootloader. Esses mecanismos específicos
não foram clonados. Aqui há um único caminho textual nativo; isso não muda o
bootloader, não instala decoder binário e não exige RTC/SNTP para logs de uptime.

## Ponto flutuante

No build de debug, CBPRINTF_FP_SUPPORT e PICOLIBC_IO_FLOAT ficam habilitados.
São preservados os formatos solicitados: %02.04f pode mostrar 1.5600, e %.2e
mostra 1.56e-03. RT_LOG_SCI é conveniência para %.2e. Usa ponto decimal padrão C;
não converte para vírgula nem normaliza todos os floats obrigatoriamente em %e.
Não há formatação de float própria. Campos dinâmicos (*), sinais, inf/nan e
mistura com inteiros/strings foram testados no formatter cbprintf do Zephyr.

O perfil de operação compila só W/E, sem cores, sem controle dinâmico e sem float,
reduzindo código/estado. Se precisar emitir floats nesse perfil, habilite
RT_LOG_FLOAT_SUPPORT e mantenha o nível da mensagem compilado. Não confundir
float para apresentação com ativação de FPU ou medição de consumo elétrico.

## Propriedade, execução e limites

A camada filtra e encaminha macros ao logger Zephyr. Formatação/transmissão seguem
o modo nativo; os firmwares usam LOG_MODE_DEFERRED, fila limitada de 16 KiB,
LOG_BLOCK_IN_THREAD=n e CDC com controle de fluxo. Não existe thread/timer/fila
adicional no módulo. Strings transitórias seguem as regras de cópia do logger
nativo; são válidas durante a chamada. Mensagens sem capacidade podem ser
descartadas pelo logger nativo, que informa drops. Não é armazenamento permanente.

Overrides usam memória fixa (16 slots no debug, 4 no perfil de operação antes
de desabilitar o controle dinâmico), sem malloc. Setter copia tags de até 31
bytes; retorna -EINVAL, -ENAMETOOLONG, -ENOSPC em falhas. Wildcard * limpa
a tabela e muda o padrão. Spinlock Zephyr protege a tabela. Com controle dinâmico
n, tabela e lock são removidos; setter retorna -ENOTSUP. Camada inteira n remove
a avaliação dos logs e seus argumentos. API C com linkage C/C++.

## Dependências e consumidores

Depende do logger/cbprintf Zephyr; não depende de coletor, reporter, cliente de
rede ou aplicação. Outros módulos podem declarar rt_log em build.depends no
manifesto Zephyr. Coleta numérica pura não deve emitir logs só por declarar essa
dependência de integração. Formatação e transmissão obedecem ao backend nativo.

Usar esta camada para mensagens de diagnóstico. snprintf usado para serializar
payload de aplicação é outra responsabilidade; não é um backend alternativo.

## Testes

ZEPHYR_BASE=/caminho/zephyr python3 tests/run.py

Filtros reais e macros da camada com transporte/lock substituídos em host;
formatter cbprintf_complete.c real do Zephyr com ASan/UBSan e warnings como erros.
Testa E/W/I/D/V, estático/dinâmico/desabilitado, limites/ownership das tags,
argumentos/tags sem avaliação quando removidos, teto local, %f/%02.04f/%e,
campos dinâmicos, valores negativos, zero, grandeza extrema, inf/nan.
Não simula fila diferida, ABI de empacotamento ARM ou concorrência do kernel;
cores/transmissão e custo precisam ser conferidos na placa. A configuração de
cores/timestamps foi testada por build com overrides desligados.

Fontes: https://docs.zephyrproject.org/latest/services/logging/index.html
https://docs.zephyrproject.org/latest/services/formatted_output.html
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/log.html

## Integração em outro projeto Zephyr

Disponibilize esta pasta como módulo (por manifesto west ou caminho local). Para
integração local, acrescente antes de `find_package(Zephyr ...)` no CMakeLists.txt:

```cmake
cmake_minimum_required(VERSION 3.20.0)

list(APPEND ZEPHYR_EXTRA_MODULES "${CMAKE_CURRENT_LIST_DIR}/../modules/rt_log")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(example)
target_sources(app PRIVATE src/main.c)
```

O caminho é relativo à aplicação do exemplo. `zephyr/module.yml` associa o
Kconfig e o CMake do módulo; não é necessário adicionar `src/log.c` manualmente.
Configuração mínima em `prj.conf` para saída UART diferida com float:

```ini
CONFIG_LOG=y
CONFIG_LOG_MODE_DEFERRED=y
CONFIG_LOG_BACKEND_UART=y
CONFIG_RT_LOG=y
CONFIG_RT_LOG_MAX_LEVEL=5
CONFIG_RT_LOG_DEFAULT_LEVEL=3
CONFIG_RT_LOG_FLOAT_SUPPORT=y
```

O formatter completo (`CBPRINTF_COMPLETE`) é dependência, não selecionado pelo
módulo. Confirme-o em `build/zephyr/.config`; desative uma eventual seleção do
formatter nano na aplicação. A placa deve oferecer console/backend UART válido.
USB CDC, pinagem, controle de fluxo e tamanho de buffers pertencem à aplicação.

## Arquitetura, ciclo de vida e decisão de emissão

`include/rt_log/log.h` contém a fachada, macros e contratos públicos; `src/log.c`
contém os filtros; Kconfig define limites e defaults de apresentação. Não há
callback público, estrutura pública mutável, protocolo de rede ou formato salvo.
A saída textual é diagnóstico humano; não há promessa de layout estável para
parser. Não existem framing, checksum, ACK, retransmissão ou versionamento de
pacotes nesta camada.

Não há init/start/stop: estado estático começa com o nível configurado e tabela
vazia. `rt_log_level_set` altera apenas RAM; reiniciar perde overrides. A ordem é:

1. O teto global elimina macros acima do nível compilado.
2. O teto local pode eliminar mais mensagens, inclusive avaliação da tag.
3. A tag é avaliada uma vez; `rt_log_is_enabled` consulta o teto efetivo.
4. Se permitido, a macro Zephyr recebe os argumentos e aplica seus próprios
   filtros, empacotamento e política de capacidade.
5. No modo diferido o consumidor nativo formata e transmite posteriormente.

Uma consulta positiva não reserva fila: outro contexto pode mudar filtros entre
consulta e emissão, e o backend pode descartar a mensagem. Não use logs como
confirmação de execução ou canal confiável de dados.

## Contratos da API e concorrência

| API | Efeito e resultado |
|---|---|
| `rt_log_level_set(tag, level)` | Copia override; retorna 0 ou errno negativo. `"*"` muda padrão e limpa tabela. |
| `rt_log_level_get(tag)` | Teto inclusivo limitado por MAX_LEVEL; tag desconhecida usa padrão, inválida retorna NONE. |
| `rt_log_is_enabled(tag, level)` | Consulta sem efeitos; rejeita NONE e nível fora do teto compilado. |
| `RT_LOG_MODULE_REGISTER(name)` | Identificador C único por unidade de tradução, em escopo de arquivo. |
| `RT_LOGE/W/I/D/V(tag, format, ...)` | Instruções sem retorno, formato literal printf; não confirmam saída. |
| `RT_LOG_SCI(tag, label, value)` | INFO, double, duas casas após ponto em notação científica. |

`rt_log_format_check`, `RT_LOG_OMIT`, `RT_LOG_DISPATCH` e `RT_LOG_NATIVE_LEVEL`
são mecanismos internos, não pontos de integração. A sentinela de formato não
tem implementação vinculável e nunca deve ser chamada diretamente.

Nomes têm até TAG_MAX_LEN **bytes**, excluindo NUL; não há truncamento nem
contagem de caracteres Unicode. O setter copia o nome. Consultas e emissão
emprestam o ponteiro pela duração da chamada; não o modifique concorrentemente.
Strings/argumentos encaminhados ao backend seguem as regras do logger Zephyr,
sem garantia de cópia profunda para dados arbitrários.

O spinlock protege padrão e tabela, permitindo consultas/alterações concorrentes
em contextos compatíveis com spinlocks Zephyr. Não usar em NMI. Não há alocação,
I/O ou espera de fila dentro da região crítica. A busca é linear e limitada por
TAG_SLOTS e TAG_MAX_LEN; não é tempo constante. Segurança e custo de emissão em
ISR dependem também do modo/backend nativo; evite formatação e argumentos caros
em interrupções. Não confundir esta limitação com uma garantia de WCET medida.

Para um laço determinístico, filtre antes de preparar snapshots caros e use o
modo diferido, sem bloqueio do produtor. Um lote de logs ainda custa CPU, memória
e tráfego USB/UART. Configure frequência e teto menores em operação. Consumo
elétrico do conjunto exige medição física, inclusive sem terminal conectado.

## Erros e solução de problemas

| Sintoma/retorno | Causa e ação |
|---|---|
| `-EINVAL` | Setter recebeu NULL, nome vazio ou enum fora de 0..5. Corrigir entrada. |
| `-ENAMETOOLONG` | Nome acima do limite em bytes. Reduzir nome ou configurar limite. |
| `-ENOSPC` | Tabela cheia para nova tag. Atualizar existente, aumentar slots ou redefinir com `"*"` (limpa todas). |
| `-ENOTSUP` | Módulo ou controle dinâmico desligado. Usar configuração estática ou habilitar a opção. |
| Sem INFO/DEBUG | Conferir MAX_LEVEL, DEFAULT_LEVEL, LOCAL_LEVEL, overrides e filtros nativos. |
| Sem saída alguma | Conferir backend, console da placa, USB/terminal e CONFIG_RT_LOG. |
| Sem cor/timestamp | Conferir opções nativas explícitas e suporte ANSI do terminal. |
| Float ausente/incorreto | Conferir formatter completo e CBPRINTF_FP_SUPPORT; passar tipos printf corretos. |
| Mensagens perdidas | Conferir drops nativos e capacidade; reduzir volume. Não há retry do rt_log. |
| Erro de concatenação de formato | Usar literal ou macro de literal, não ponteiro de formato em runtime. |

As unidades dos valores são responsabilidade de quem emite: explicite `_us`,
`_ms`, volts etc. O módulo não converte unidades. Severidade deve refletir o
**evento**: um estado periódico com campo `last_error` continua INFO quando não
há uma nova falha. Não escolher ERROR por palavras contidas no formato.

## Defaults e custo de configuração

| Opção | Default / faixa | Custo ou interação |
|---|---|---|
| RT_LOG | n | Depende de LOG e CBPRINTF_COMPLETE. |
| MAX_LEVEL / DEFAULT_LEVEL | 4 / 4; 0..5 | Compilação / teto runtime; efetivo é limitado por MAX. |
| DYNAMIC_LEVEL_CONTROL | y | Tabela e spinlock; n remove ambos. |
| TAG_SLOTS | 16; 1..64 | Entradas fixas, busca linear; padrão não ocupa slot. |
| TAG_MAX_LEN | 31; 8..63 bytes | Cada entrada reserva limite + NUL, enum e alinhamento. |
| TIMESTAMP / FORMAT_TIMESTAMP | y / y | Segundo depende do primeiro; apresentação nativa. |
| SHOW_LEVEL / COLOR | y / y | Acrescentam prefixo/sequências ANSI à saída. |
| FLOAT_SUPPORT | y | Seleciona formatter float; n não desativa seleção feita por outro módulo. |

O espaço exato da tabela é `TAG_SLOTS * sizeof(struct tag_filter)`, dependente do
ABI. Fila e stack do logger são recursos nativos adicionais, fora dessa conta.

## Compatibilidade e validação

Não houve remoção de API nesta revisão de documentação. A compatibilidade é com
a fachada descrita neste cabeçalho, sem promessa de ABI/API integral ESP-IDF.
O módulo não tem dependência de placa; integração atual usa Zephyr 4.4 de
desenvolvimento. Atualizar Zephyr exige repetir testes do formatter e builds.

Na raiz deste repositório, com o ambiente Zephyr ativo:

```sh
ZEPHYR_BASE=/caminho/zephyr python3 tests/run.py
west build -b SUA_PLACA -d build examples/basic
```

O runner exige macOS/Xcode e clang. Quatro variantes: máximo 5 dinâmico, máximo 2
dinâmico, máximo 5 estático e módulo desligado. examples/basic é uma aplicação
completa sem módulos de diagnóstico. tests/backend_options.conf permite validar
os defaults de apresentação desligados com EXTRA_CONF_FILE, em build separado.

Testes host substituem transporte/lock, não o formatter. Não validam fila USB,
concorrência kernel/ISR ou empacotamento ARM real. Saída decimal/científica foi
observada em consumidores físicos; custo elétrico não foi medido. Evidência e
limites em docs/VALIDATION.md. Nenhum comando de teste acima grava a placa.

## Repositório independente

Repositório: [smartsensingme/rt_log](https://github.com/smartsensingme/rt_log).
Versão 0.1.0, licença Apache-2.0. Esta pasta é a unidade de reutilização: CMakeLists.txt, Kconfig, include/, src/, tests/ e zephyr/module.yml.
Adicionar sua pasta a ZEPHYR_EXTRA_MODULES antes de find_package(Zephyr), ou fornecer
por manifesto west. Não depende dos módulos de diagnóstico ou da aplicação.
Caminhos usados no projeto consumidor não fazem parte da API desta biblioteca.

Os testes exigem ZEPHYR_BASE apontando a um checkout compatível para compilar o
formatter real: ZEPHYR_BASE=/caminho/zephyr python3 tests/run.py. O runner atual
foi validado em macOS/Xcode; não se declara execução validada em outros sistemas.
Ver CHANGELOG.md e docs/VALIDATION.md para compatibilidade e limites.
