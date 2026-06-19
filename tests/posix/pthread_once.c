#include <assert.h>
#include <pthread.h>
#include <unistd.h>

static pthread_once_t once = PTHREAD_ONCE_INIT;
static int init_count = 0;
static int entered = 0;
static int release_init = 0;

static void init_once(void) {
	__atomic_fetch_add(&init_count, 1, __ATOMIC_SEQ_CST);
	__atomic_store_n(&entered, 1, __ATOMIC_SEQ_CST);
	while (!__atomic_load_n(&release_init, __ATOMIC_SEQ_CST))
		usleep(1000);
}

static void *worker(void *arg) {
	(void)arg;
	assert(!pthread_once(&once, init_once));
	return NULL;
}

int main(void) {
	pthread_t t1;
	pthread_t t2;

	assert(!pthread_create(&t1, NULL, worker, NULL));
	while (!__atomic_load_n(&entered, __ATOMIC_SEQ_CST))
		usleep(1000);

	assert(!pthread_create(&t2, NULL, worker, NULL));
	usleep(10000);
	assert(__atomic_load_n(&init_count, __ATOMIC_SEQ_CST) == 1);

	__atomic_store_n(&release_init, 1, __ATOMIC_SEQ_CST);
	assert(!pthread_join(t1, NULL));
	assert(!pthread_join(t2, NULL));
	assert(__atomic_load_n(&init_count, __ATOMIC_SEQ_CST) == 1);

	assert(!pthread_once(&once, init_once));
	assert(__atomic_load_n(&init_count, __ATOMIC_SEQ_CST) == 1);

	return 0;
}
