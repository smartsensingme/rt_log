/* SPDX-License-Identifier: Apache-2.0 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <zephyr/sys/cbprintf.h>

#include <rt_log/log.h>

RT_LOG_MODULE_REGISTER(test_logs);

static char output[8192];
static size_t used;
static unsigned native_calls, tag_calls;

/* Callback interno de cbvprintf, instalado por test_printk; acrescenta um byte e NUL ao buffer
 * limitado, abortando por assert se excedido. */
static int append(int c, void *ctx)
{
	(void)ctx;
	assert(used + 1 < sizeof(output));
	output[used++] = (char)c;
	output[used] = 0;

	return c;
}

/* Entrada do backend host substituto, chamada pelas macros LOG_* de teste; chama cbvprintf real com
 * append, retorna contagem nativa. Não é API embarcada. */
int test_printk(const char *fmt, ...)
{
	++native_calls;

	va_list args;
	va_start(args, fmt);
	int n = cbvprintf(append, NULL, fmt, args);
	va_end(args);

	return n;
}

/* Helper interno de main: retorna nome estático e conta avaliações da expressão de tag. */
static const char *tag(void)
{
	tag_calls++;
	return "TEST";
}

/* Helper interno de main: limpa captura e contagem de saídas; preserva contador de avaliações de
 * tag. */
static void clear(void)
{
	used = 0;
	output[0] = 0;
	native_calls = 0;
}

void test_local_filter(void);

/* Entrada externa do runner host; chama test_local_filter e verifica filtros/formatos/erros nas
 * quatro configurações. Assert falha encerra o processo. */
int main(void)
{
	test_local_filter();

	clear();
	int effects = 0;

#if !defined(CONFIG_RT_LOG) || !CONFIG_RT_LOG
	RT_LOGE(tag(), "hidden=%d", ++effects);
	RT_LOG_SCI(tag(), "value", ++effects);
	assert(!effects && !tag_calls && !native_calls);
	assert(rt_log_level_set("*", RT_LOG_INFO) == -ENOTSUP &&
	       rt_log_level_get("T") == RT_LOG_NONE);
	assert(!rt_log_is_enabled("T", RT_LOG_INFO));

	puts("PASS: disabled facade removes output, tag/argument evaluation and runtime state");
#else
#if !CONFIG_RT_LOG_DYNAMIC_LEVEL_CONTROL
	assert(rt_log_level_set("*", RT_LOG_WARN) == -ENOTSUP);
	assert(rt_log_level_get("TEST") == RT_LOG_DEBUG);
	RT_LOGI(tag(), "static_filter=%d", ++effects);
	assert(effects == 1 && tag_calls == 1 && native_calls == 1);
	RT_LOGV("TEST", "removed=%d", ++effects);
	assert(effects == 1);

	puts("PASS: static runtime level, no override table/setter, verbose filtered");
	return 0;
#endif

	assert(!rt_log_level_set("*", RT_LOG_WARN));
	RT_LOGI(tag(), "hidden=%d", ++effects);
	assert(!effects && !native_calls);
#if CONFIG_RT_LOG_MAX_LEVEL >= 3
	assert(tag_calls == 1); /* one runtime tag evaluation, arguments remain lazy */
#else
	assert(!tag_calls); /* compile-time removal also removes tag evaluation */
#endif
	RT_LOGW("TEST", "warning=%d", ++effects);
	assert(effects == 1 && native_calls == 1 && strstr(output, "warning=1"));
	assert(!rt_log_level_set("TEST", RT_LOG_VERBOSE));

	clear();
	RT_LOGE("TEST", "error");
	assert(strstr(output, "error"));
#if CONFIG_RT_LOG_MAX_LEVEL >= 3

	clear();
	RT_LOGI("TEST", "decimal=%02.04f scientific=%.2e", 1.56, 0.00156);
	assert(strstr(output, "decimal=1.5600 scientific=1.56e-03"));

	clear();
	RT_LOG_SCI("TEST", "small", 0.00156);
	assert(strstr(output, "small=1.56e-03"));

	clear();
	RT_LOGI("TEST", "negative=%.2e zero=%.2e huge=%.2e", -0.00156, 0.0, 1e100);
	assert(strstr(output, "negative=-1.56e-03 zero=0.00e+00 huge=1.00e+100"));

	clear();
	RT_LOGI("TEST", "dynamic=%*.*f suffix=%d text=%s percent=%%", 8, 3, 1.25, 42, "ok");
	assert(strstr(output, "   1.250 suffix=42 text=ok percent=%"));

	clear();
	RT_LOGI("TEST", "nan=%f inf=%e", NAN, INFINITY);
	assert(strstr(output, "nan=nan") && strstr(output, "inf=inf"));
#endif
#if CONFIG_RT_LOG_MAX_LEVEL >= 4

	clear();
	RT_LOGD("TEST", "debug");
	assert(strstr(output, "debug"));
#endif
#if CONFIG_RT_LOG_MAX_LEVEL >= 5

	clear();
	RT_LOGV("TEST", "verbose");
	assert(strstr(output, "[V] verbose"));
#endif
	assert(!rt_log_level_set("TEST", RT_LOG_NONE));

	clear();
	RT_LOGW("TEST", "hidden=%d", ++effects);
	assert(!native_calls && effects == 1);

	assert(!rt_log_level_set("*", RT_LOG_INFO));

	char mutable_tag[] = "COPY";
	assert(!rt_log_level_set(mutable_tag, RT_LOG_ERROR));
	mutable_tag[0] = 'X';
	assert(rt_log_level_get("COPY") == RT_LOG_ERROR);
	assert(!rt_log_is_enabled(NULL, RT_LOG_ERROR) && !rt_log_is_enabled("", RT_LOG_ERROR));
	assert(!rt_log_is_enabled("TEST", RT_LOG_NONE) &&
	       !rt_log_is_enabled("TEST", (enum rt_log_level) - 1));
	assert(rt_log_level_set(NULL, RT_LOG_INFO) == -EINVAL &&
	       rt_log_level_set("", RT_LOG_INFO) == -EINVAL);
	assert(rt_log_level_set("TEST", (enum rt_log_level) - 1) == -EINVAL);
	assert(rt_log_level_set("this_tag_is_longer_than_the_test_limit", RT_LOG_INFO) ==
	       -ENAMETOOLONG);

	assert(!rt_log_level_set("*", RT_LOG_INFO));
	assert(!rt_log_level_set("A", RT_LOG_ERROR));
	assert(!rt_log_level_set("B", RT_LOG_ERROR));
	assert(!rt_log_level_set("C", RT_LOG_ERROR));
	assert(!rt_log_level_set("D", RT_LOG_ERROR));
	assert(rt_log_level_set("E", RT_LOG_ERROR) == -ENOSPC);
	assert(!rt_log_level_set("A", RT_LOG_WARN) && rt_log_level_get("A") == RT_LOG_WARN);

	assert(!rt_log_level_set("*", RT_LOG_NONE) && rt_log_level_get("A") == RT_LOG_NONE);
	assert(!rt_log_level_set("E", RT_LOG_INFO));

	printf("PASS: real tag/severity filters max=%d, bounded copied tags, lazy arguments; "
	       "native Zephyr cbprintf formats floats\n",
	       CONFIG_RT_LOG_MAX_LEVEL);
#endif
	return 0;
}
