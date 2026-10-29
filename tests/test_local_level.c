/* SPDX-License-Identifier: Apache-2.0 */
#define RT_LOG_LOCAL_LEVEL RT_LOG_WARN
#include <assert.h>

#include <rt_log/log.h>

RT_LOG_MODULE_REGISTER(local_level);

static unsigned tag_calls;

/* Helper interno de test_local_filter: contador comprova ausência de avaliação. */
static const char *local_tag(void)
{
	tag_calls++;
	return "LOCAL";
}

/* Chamado externamente por main do teste: teto WARN deve eliminar I/D/V
 * sem avaliar tags ou argumentos, independentemente do teto global. */
void test_local_filter(void)
{
	int effects = 0;

	RT_LOGI(local_tag(), "info=%d", ++effects);
	RT_LOGD(local_tag(), "debug=%d", ++effects);
	RT_LOGV(local_tag(), "verbose=%d", ++effects);

	assert(!effects && !tag_calls);
}
