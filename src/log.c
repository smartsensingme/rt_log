/* SPDX-License-Identifier: Apache-2.0 */

#include <stddef.h>
#include <string.h>

#include <zephyr/spinlock.h>

#include <rt_log/log.h>

#if defined(CONFIG_RT_LOG_DYNAMIC_LEVEL_CONTROL) && CONFIG_RT_LOG_DYNAMIC_LEVEL_CONTROL

/** Entrada privada: tag copiada (bytes + NUL), vazia indica slot livre;
 * level guarda o teto solicitado, antes do clamp pelo teto compilado. */
struct tag_filter {
	char tag[CONFIG_RT_LOG_TAG_MAX_LEN + 1]; /**< Nome próprio, sem ponteiro externo. */
	enum rt_log_level level;		 /**< Teto inclusivo 0..5. */
};

static struct tag_filter tags[CONFIG_RT_LOG_TAG_SLOTS];
static enum rt_log_level default_level = CONFIG_RT_LOG_DEFAULT_LEVEL;
static struct k_spinlock filter_lock;
#endif

/** Uso interno por rt_log_level_set/get. Conta bytes até limite + 1 para
 * detectar excesso sem varrer uma string inteira; NULL retorna 0.
 * Ponteiro não NULL deve referenciar string válida; não valida memória
 * arbitrária. Sem estado, lock ou alocação; retorno em bytes, não caracteres
 * Unicode. */
static size_t tag_length(const char *tag)
{
	/* Leitura limitada distingue nome vazio, válido e excessivo. */
	size_t n = 0;

	if (!tag)
		return 0;

	while (n <= CONFIG_RT_LOG_TAG_MAX_LEN && tag[n])
		++n;

	return n;
}

/** API externa, sem chamadores internos; contrato completo em log.h.
 * Valida antes de bloquear, procura entrada existente ou primeiro slot livre,
 * copia a tag somente ao inserir. Chama tag_length e primitivas de spinlock.
 */
int rt_log_level_set(const char *tag, enum rt_log_level level)
{
#if !defined(CONFIG_RT_LOG_DYNAMIC_LEVEL_CONTROL) || !CONFIG_RT_LOG_DYNAMIC_LEVEL_CONTROL
	/* Perfil estático não mantém tabela mutável. */
	(void)tag;
	(void)level;

	return -ENOTSUP;
#else
	/* Nenhuma falha de entrada deve alterar a configuração vigente. */
	size_t len = tag_length(tag);

	if (!len || (unsigned)level > RT_LOG_VERBOSE)
		return -EINVAL;
	if (len > CONFIG_RT_LOG_TAG_MAX_LEN)
		return -ENAMETOOLONG;

	/* Região crítica limitada: nenhum log, I/O ou alocação sob o lock. */
	k_spinlock_key_t key = k_spin_lock(&filter_lock);

	/* Wildcard redefine tudo atomicamente em relação às consultas. */
	if (!strcmp(tag, "*")) {
		default_level = level;
		memset(tags, 0, sizeof(tags));
		k_spin_unlock(&filter_lock, key);

		return 0;
	}

	/* Priorizar atualização existente mesmo quando a tabela estiver cheia. */
	size_t free_slot = CONFIG_RT_LOG_TAG_SLOTS;

	for (size_t i = 0; i < CONFIG_RT_LOG_TAG_SLOTS; i++) {
		if (!tags[i].tag[0]) {
			if (free_slot == CONFIG_RT_LOG_TAG_SLOTS)
				free_slot = i;
		} else if (!strcmp(tags[i].tag, tag)) {
			tags[i].level = level;
			k_spin_unlock(&filter_lock, key);
			return 0;
		}
	}

	if (free_slot == CONFIG_RT_LOG_TAG_SLOTS) {
		k_spin_unlock(&filter_lock, key);

		return -ENOSPC;
	}

	/* Publicar cópia própria e nível sob o mesmo lock. */
	memcpy(tags[free_slot].tag, tag, len + 1);
	tags[free_slot].level = level;
	k_spin_unlock(&filter_lock, key);

	return 0;
#endif
}

/** Uso externo e por rt_log_is_enabled; consulta sem mutação.
 * tag_length valida o nome; a busca protegida obtém um snapshot do teto.
 * Alterações posteriores ao unlock só afetam consultas seguintes.
 */
enum rt_log_level rt_log_level_get(const char *tag)
{
	/* Nome inválido nunca herda o nível padrão. */
	size_t len = tag_length(tag);

	if (!len || len > CONFIG_RT_LOG_TAG_MAX_LEN)
		return RT_LOG_NONE;
#if defined(CONFIG_RT_LOG_DYNAMIC_LEVEL_CONTROL) && CONFIG_RT_LOG_DYNAMIC_LEVEL_CONTROL
	/* Snapshot consistente da tabela e do padrão dinâmico. */
	k_spinlock_key_t key = k_spin_lock(&filter_lock);
	enum rt_log_level level = default_level;

	for (size_t i = 0; i < CONFIG_RT_LOG_TAG_SLOTS; i++)
		if (tags[i].tag[0] && !strcmp(tags[i].tag, tag)) {
			level = tags[i].level;
			break;
		}

	k_spin_unlock(&filter_lock, key);
#else
	enum rt_log_level level = CONFIG_RT_LOG_DEFAULT_LEVEL;
#endif
	/* Configuração runtime não recompila mensagens eliminadas. */
	return level > CONFIG_RT_LOG_MAX_LEVEL ? CONFIG_RT_LOG_MAX_LEVEL : level;
}

/** Uso externo e pelo despacho E/W/I/D/V; chama rt_log_level_get.
 * Apenas decisão de filtro: sem reserva na fila e sem confirmação do backend.
 */
bool rt_log_is_enabled(const char *tag, enum rt_log_level level)
{
	/* Cast também rejeita enums negativos antes da consulta de tag. */
	if ((unsigned)level > CONFIG_RT_LOG_MAX_LEVEL || level == RT_LOG_NONE)
		return false;

	return level <= rt_log_level_get(tag);
}
