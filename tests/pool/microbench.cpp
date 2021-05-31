#include "workForC.hpp"

#include <pthread.h>

int main(int argc, char** argv) {
	pthread_spinlock_t lock;

	pthread_spin_init(&lock, 0);

	struct myFargs* args = (struct myFargs*)malloc(sizeof(struct myFargs));
	args->iters = atoi(argv[1]);
	args->task_id = 0;
	args->lock = &lock;

	pthread_spin_lock(&lock);
	myF((void*)args);
}
