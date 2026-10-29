#ifndef RT_LOG_TEST_SPINLOCK_H
#define RT_LOG_TEST_SPINLOCK_H
/* Sequential policy tests only; real target uses Zephyr's ISR-safe spinlock. */
struct k_spinlock {
	int unused;
};

typedef int k_spinlock_key_t;

static inline int k_spin_lock(struct k_spinlock *l)
{
	(void)l;
	return 0;
}

static inline void k_spin_unlock(struct k_spinlock *l, int k)
{
	(void)l;
	(void)k;
}
#endif
