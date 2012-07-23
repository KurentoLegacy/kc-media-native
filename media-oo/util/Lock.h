
#ifndef __LOCK_H__
#define __LOCK_H__

#include <pthread.h>

class Lock {
public:
	Lock() { pthread_mutex_init(&plock, NULL); }
	~Lock() { pthread_mutex_unlock(&plock); }

	void lock() { pthread_mutex_lock(&plock); }
	void unlock() { pthread_mutex_unlock(&plock); }

private:
	pthread_mutex_t plock;
};

#endif /* __LOCK_H__ */
