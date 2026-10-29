/* Formatter-only host test: no scheduler/CPU ABI packaging is exercised.
 * cbprintf_complete uses the host compiler va_list; do not include Zephyr kernel structs. */
#ifndef RT_LOG_TEST_ARCH_CPU_H
#define RT_LOG_TEST_ARCH_CPU_H
#endif
