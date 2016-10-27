
pthread_mutex_t esbmc_mutex = PTHREAD_MUTEX_INITIALIZER;

#define __ESBMC_atomic_begin() pthread_mutex_lock(&esbmc_mutex)
#define __ESBMC_atomic_end() pthread_mutex_unlock(&esbmc_mutex)



