/* SPDX-License-Identifier: Apache-2.0 */
#include <zephyr/kernel.h>

#include <rt_log/log.h>

RT_LOG_MODULE_REGISTER(log_example);

/** Zephyr entry; called externally at boot. Demonstrates native formatting and
 * runtime tag filtering without any diagnostics/client/board library.
 * Calls rt_log_level_set, RT_LOGI/SCI/D and kernel sleep; no retained pointers.
 */
int main(void)
{
	int rc = rt_log_level_set("EXAMPLE", RT_LOG_INFO);
	if (rc)
		return rc;

	for (;;) {
		RT_LOGI("EXAMPLE", "decimal=%02.04f", 1.56);
		RT_LOG_SCI("EXAMPLE", "current_a", 0.00156);
		RT_LOGD("EXAMPLE", "filtered until runtime level allows DEBUG");
		k_msleep(5000);
	}
}
