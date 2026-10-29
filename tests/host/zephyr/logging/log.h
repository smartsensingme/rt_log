#ifndef RT_LOG_TEST_NATIVE_H
#define RT_LOG_TEST_NATIVE_H

#define LOG_MODULE_REGISTER(name, level)
#define LOG_ERR(fmt, ...) test_printk("[E] " fmt "\n", ##__VA_ARGS__)
#define LOG_WRN(fmt, ...) test_printk("[W] " fmt "\n", ##__VA_ARGS__)
#define LOG_INF(fmt, ...) test_printk("[I] " fmt "\n", ##__VA_ARGS__)
#define LOG_DBG(fmt, ...) test_printk("[D] " fmt "\n", ##__VA_ARGS__)

int test_printk(const char *fmt, ...);
#endif
