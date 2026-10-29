/* SPDX-License-Identifier: Apache-2.0 */
#ifndef RT_LOG_H
#define RT_LOG_H

/**
 * @file
 * @brief API C/C++ de logs por tag sobre o logger Zephyr.
 *
 * Uso externo: registrar um módulo por unidade de tradução e emitir
 * RT_LOGE/W/I/D/V. Não há inicialização própria. O backend e o modo de
 * processamento são do Zephyr. Tags são strings terminadas em NUL, não vazias,
 * com até TAG_MAX_LEN bytes; mantê-las estáveis durante a chamada. Formatos das
 * macros devem ser literais. Em modo diferido, captura/cópia dos argumentos
 * segue o empacotador nativo: não presumir cópia profunda de ponteiros
 * arbitrários. Não passar credenciais.
 */

#include <errno.h>
#include <stdbool.h>

/** Teto inclusivo de severidade; valores ordinais sem unidade, de 0 a 5. */
enum rt_log_level {
	RT_LOG_NONE,   /**< 0: silencia todas as mensagens. */
	RT_LOG_ERROR,  /**< 1: falha que exige atenção; backend ERR. */
	RT_LOG_WARN,   /**< 2: condição anormal recuperável; backend WRN. */
	RT_LOG_INFO,   /**< 3: evento operacional/estado periódico; backend INF. */
	RT_LOG_DEBUG,  /**< 4: detalhe de diagnóstico; backend DBG. */
	RT_LOG_VERBOSE /**< 5: detalhe adicional, backend DBG e marcador [V]. */
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Sentinela de checagem printf, de uso interno por RT_LOG_OMIT.
 * @param tag Tag referenciada apenas para checagem, sem avaliação em execução.
 * @param format Formato printf; argumentos variádicos devem corresponder aos
 * tipos. Não chamar externamente: não possui definição vinculável. A chamada
 * fica em if (0), preservando diagnósticos de compilação sem código nem efeitos
 * colaterais.
 */
#if defined(__GNUC__)
void rt_log_format_check(const char *tag, const char *format, ...)
    __attribute__((format(printf, 2, 3)));
#else
void rt_log_format_check(const char *tag, const char *format, ...);
#endif
#ifdef __cplusplus
}
#endif

/** Teto externo opcional por arquivo (0..5), definido antes do include.
 * Só restringe o teto global; não altera a tabela de filtros em execução. */
#ifndef RT_LOG_LOCAL_LEVEL
#if defined(CONFIG_RT_LOG_MAX_LEVEL)
#define RT_LOG_LOCAL_LEVEL CONFIG_RT_LOG_MAX_LEVEL
#else
#define RT_LOG_LOCAL_LEVEL 0
#endif
#endif

/** Macro interna usada por E/W/I/D/V removidos: valida formato sem avaliar
 * nada. tag/fmt/variádicos não são executados; retorna como uma instrução sem
 * valor. */
#define RT_LOG_OMIT(tag, fmt, ...)                                                                 \
	do {                                                                                       \
		if (0)                                                                             \
			rt_log_format_check((tag), (fmt), ##__VA_ARGS__);                          \
	} while (0)
#if defined(CONFIG_RT_LOG) && CONFIG_RT_LOG
#include <zephyr/logging/log.h>

/** Teto interno do módulo Zephyr: VERBOSE compartilha o nível DBG (4). */
#if CONFIG_RT_LOG_MAX_LEVEL > 4
#define RT_LOG_NATIVE_LEVEL 4
#else
#define RT_LOG_NATIVE_LEVEL CONFIG_RT_LOG_MAX_LEVEL
#endif

/** Registro externo, uma vez por .c/.cpp, em escopo de arquivo.
 * name é identificador C único no projeto, não a tag textual de mensagens.
 * Usa o logger nativo; não cria tarefa, fila ou inicializador do rt_log. */
#define RT_LOG_MODULE_REGISTER(name) LOG_MODULE_REGISTER(name, RT_LOG_NATIVE_LEVEL)
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Define teto de uma tag ou redefine padrão e limpa overrides com "*".
 * Chamada externamente; não tem chamadores internos de produção.
 * @param tag Nome válido; copiado na tabela, sem reter o ponteiro do chamador.
 * @param level Teto 0..5. Pode exceder MAX_LEVEL, mas não recupera logs
 * removidos.
 * @return 0; -EINVAL para tag vazia/NULL ou nível inválido; -ENAMETOOLONG
 * para nome excessivo; -ENOSPC sem slot livre; -ENOTSUP com controle
 * desativado. Falhas não alteram estado. Atualizar tag existente funciona com
 * tabela cheia. Concorrente com get/set sob spinlock curto; busca O(TAG_SLOTS *
 * TAG_MAX_LEN), sem alocação/espera de I/O. Não usar em NMI; não garante tempo
 * constante.
 */
int rt_log_level_set(const char *tag, enum rt_log_level level);

/**
 * @brief Consulta teto efetivo, limitado por MAX_LEVEL, sem alterar estado.
 * Uso externo e interno por rt_log_is_enabled; chama tag_length.
 * @param tag Nome válido, emprestado somente durante a chamada.
 * @return Override ou padrão limitado ao teto compilado; NONE para tag inválida
 * ou módulo desativado. Não consulta filtros adicionais do backend Zephyr.
 * Busca limitada, protegida por spinlock se dinâmica; nenhuma alocação/I/O.
 */
enum rt_log_level rt_log_level_get(const char *tag);

/**
 * @brief Consulta se a camada permite um nível para uma tag.
 * Uso externo (evitar preparar diagnóstico caro) e interno por RT_LOG_DISPATCH;
 * chama rt_log_level_get. Não reserva espaço na fila nem garante transmissão.
 * @param tag Nome emprestado durante a chamada, com as regras de level_get.
 * @param level Severidade 1..MAX_LEVEL; NONE e valores fora do teto são
 * negados.
 * @return true se permitido pela camada; false se inválido, filtrado ou
 * desligado. Sem mudança de estado. Mesmas restrições de concorrência de
 * level_get.
 */
bool rt_log_is_enabled(const char *tag, enum rt_log_level level);
#ifdef __cplusplus
}
#endif

/** Despacho interno das macros de emissão: verifica teto local, avalia tag uma
 * vez, consulta filtro e só então entrega argumentos ao macro nativo. Sem
 * retorno. native/level são selecionados pelos wrappers; fmt deve expandir em
 * literal. */
#define RT_LOG_DISPATCH(native, level, tag, fmt, ...)                                              \
	do {                                                                                       \
		if ((level) <= RT_LOG_LOCAL_LEVEL) {                                               \
			const char *const _rt_log_tag = (tag);                                     \
			if (rt_log_is_enabled(_rt_log_tag, (level)))                               \
				native("%s: " fmt, _rt_log_tag, ##__VA_ARGS__);                    \
		}                                                                                  \
	} while (0)
#else
/* Módulo desligado: registro vazio e consultas externas inertes. */
#define RT_LOG_MODULE_REGISTER(name)
static inline int rt_log_level_set(const char *t, enum rt_log_level l)
{
	(void)t;
	(void)l;
	return -ENOTSUP;
}

static inline enum rt_log_level rt_log_level_get(const char *t)
{
	(void)t;
	return RT_LOG_NONE;
}

static inline bool rt_log_is_enabled(const char *t, enum rt_log_level l)
{
	(void)t;
	(void)l;
	return false;
}
#endif

/** Emissão externa ERROR/ERR. t: tag emprestada; f: literal printf;
 * variádicos com tipos compatíveis. Sem retorno ou confirmação de entrega.
 * Chama RT_LOG_DISPATCH ou RT_LOG_OMIT conforme teto compilado. Não colocar
 * efeitos necessários à aplicação nos argumentos: filtros podem suprimi-los.
 * Custo/segurança em ISR e cópia de strings dependem do logger nativo
 * configurado.
 */
#if defined(CONFIG_RT_LOG) && CONFIG_RT_LOG && CONFIG_RT_LOG_MAX_LEVEL >= 1
#define RT_LOGE(t, f, ...) RT_LOG_DISPATCH(LOG_ERR, RT_LOG_ERROR, t, f, ##__VA_ARGS__)
#else
#define RT_LOGE(t, f, ...) RT_LOG_OMIT(t, f, ##__VA_ARGS__)
#endif

/** Emissão externa WARN/WRN. t: tag emprestada; f: literal printf;
 * variádicos com tipos compatíveis. Sem retorno ou confirmação de entrega.
 * Chama RT_LOG_DISPATCH ou RT_LOG_OMIT conforme teto compilado. Não colocar
 * efeitos necessários à aplicação nos argumentos: filtros podem suprimi-los.
 * Custo/segurança em ISR e cópia de strings dependem do logger nativo
 * configurado.
 */
#if defined(CONFIG_RT_LOG) && CONFIG_RT_LOG && CONFIG_RT_LOG_MAX_LEVEL >= 2
#define RT_LOGW(t, f, ...) RT_LOG_DISPATCH(LOG_WRN, RT_LOG_WARN, t, f, ##__VA_ARGS__)
#else
#define RT_LOGW(t, f, ...) RT_LOG_OMIT(t, f, ##__VA_ARGS__)
#endif

/** Emissão externa INFO/INF. t: tag emprestada; f: literal printf;
 * variádicos com tipos compatíveis. Sem retorno ou confirmação de entrega.
 * Chama RT_LOG_DISPATCH ou RT_LOG_OMIT conforme teto compilado. Não colocar
 * efeitos necessários à aplicação nos argumentos: filtros podem suprimi-los.
 * Custo/segurança em ISR e cópia de strings dependem do logger nativo
 * configurado.
 */
#if defined(CONFIG_RT_LOG) && CONFIG_RT_LOG && CONFIG_RT_LOG_MAX_LEVEL >= 3
#define RT_LOGI(t, f, ...) RT_LOG_DISPATCH(LOG_INF, RT_LOG_INFO, t, f, ##__VA_ARGS__)
#else
#define RT_LOGI(t, f, ...) RT_LOG_OMIT(t, f, ##__VA_ARGS__)
#endif

/** Emissão externa DEBUG/DBG. t: tag emprestada; f: literal printf;
 * variádicos com tipos compatíveis. Sem retorno ou confirmação de entrega.
 * Chama RT_LOG_DISPATCH ou RT_LOG_OMIT conforme teto compilado. Não colocar
 * efeitos necessários à aplicação nos argumentos: filtros podem suprimi-los.
 * Custo/segurança em ISR e cópia de strings dependem do logger nativo
 * configurado.
 */
#if defined(CONFIG_RT_LOG) && CONFIG_RT_LOG && CONFIG_RT_LOG_MAX_LEVEL >= 4
#define RT_LOGD(t, f, ...) RT_LOG_DISPATCH(LOG_DBG, RT_LOG_DEBUG, t, f, ##__VA_ARGS__)
#else
#define RT_LOGD(t, f, ...) RT_LOG_OMIT(t, f, ##__VA_ARGS__)
#endif

/** Emissão externa VERBOSE/DBG, com marcador [V]. t: tag emprestada; f: literal
 * printf; variádicos com tipos compatíveis. Sem retorno ou confirmação de
 * entrega. Chama RT_LOG_DISPATCH ou RT_LOG_OMIT conforme teto compilado. Não
 * colocar efeitos necessários à aplicação nos argumentos: filtros podem
 * suprimi-los. Custo/segurança em ISR e cópia de strings dependem do logger
 * nativo configurado.
 */
#if defined(CONFIG_RT_LOG) && CONFIG_RT_LOG && CONFIG_RT_LOG_MAX_LEVEL >= 5
#define RT_LOGV(t, f, ...) RT_LOG_DISPATCH(LOG_DBG, RT_LOG_VERBOSE, t, "[V] " f, ##__VA_ARGS__)
#else
#define RT_LOGV(t, f, ...) RT_LOG_OMIT(t, f, ##__VA_ARGS__)
#endif

/** Emissão externa INFO de label=value em %.2e, sem unidade adicionada.
 * tag/label são strings emprestadas; value é convertido para double.
 * Chama RT_LOGI; requer float nativo habilitado para resultado numérico.
 * Ex.: 0.00156 produz 1.56e-03, com ponto decimal C, sem parser próprio. */
#define RT_LOG_SCI(tag, label, value) RT_LOGI(tag, "%s=%.2e", (label), (double)(value))
#endif
