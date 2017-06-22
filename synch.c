#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "spinlock.h"
#include "proc.h"
#include "synch.h"

int nextM = 1;
int nextC = 1;

int mutex_init(struct mutex_t *mutex){
	if(!mutex)
		return -1;
	if(mutex->id)
		return -2;

	int i;

	mutex->id = nextM++;
	mutex->flag = 0;
	mutex->tid = 0;
	mutex->waitCnt = 0;
	initlock(&mutex->lock, "mutex");

	for(i = 0; i < 7; i++){
		mutex->wait[i] = 0;
	}

	return 0;
}

int mutex_lock(struct mutex_t *mutex){
	if(!mutex)
		return -1;
	if(mutex->id < 1)
		return -2;
	if(mutex->tid == proc->tid)
		return -3;

	acquire(&mutex->lock);

	int i;

	if(mutex->flag){
		for(i = 0; i < 7; i++){
			if(!mutex->wait[i]){
				mutex->wait[i] = proc;
				mutex->waitCnt++;
				break;
			}
		}
		while(1){
			sleep(proc, &mutex->lock);
			if(mutex->wait[i] != proc)
				break;
		}
	}

	mutex->flag = 1;
	mutex->tid = proc->tid;

	release(&mutex->lock);

	return 0;
}

int mutex_unlock(struct mutex_t *mutex){
	if(!mutex)
		return -1;
	if(mutex->id < 1)
		return -2;
	if(mutex->tid != proc->tid || mutex->flag == 0)
		return -3;

	acquire(&mutex->lock);

	int i, min = 100, idx = -1;
	void *unblock = 0;

	if(mutex->waitCnt){
		for(i = 0; i < 7; i++){
			if(mutex->wait[i] && min > mutex->wait[i]->nice){
				min = mutex->wait[i]->nice;
				unblock = (void*)mutex->wait[i];
				idx = i;
			}
		}

		mutex->waitCnt--;
		mutex->wait[idx] = 0;

		wakeup(unblock);
	}

	mutex->flag = 0;
	mutex->tid = -1;

	release(&mutex->lock);

	return 0;
}

int cond_init(struct cond_t *cond){
	if(!cond)
		return -1;
	if(cond->id)
		return -2;

	int i;

	cond->id = nextC++;
	cond->waitCnt = 0;
	initlock(&cond->lock, "cond");

	for(i = 0; i < 7; i++){
		cond->wait[i] = 0;
	}

	return 0;
}

int cond_wait(struct cond_t *cond, struct mutex_t *mutex){
	if(!mutex || !cond)
		return -1;
	if(cond->id < 1 || mutex->id < 1)
		return -2;
	if(mutex->tid != proc->tid)
		return -3;

	acquire(&cond->lock);

	int i;

	for(i = 0; i < 7; i++){
		if(cond->wait[i] == 0){
			cond->wait[i] = proc;
			cond->waitCnt++;
			break;
		}
	}

	mutex_unlock(mutex);

	while(1){
		sleep(proc, &cond->lock);
		if(cond->wait[i] != proc)
			break;
	}

	release(&cond->lock);
	mutex_lock(mutex);

	return 0;
}

int cond_signal(struct cond_t *cond){
	if(!cond)
		return -1;
	if(cond->id < 1)
		return -2;

	acquire(&cond->lock);

	int i, min = 100, idx = -1;
	void *unblock = 0;

	if(cond->waitCnt){
		for(i = 0; i < 7; i++){
			if(cond->wait[i] && min > cond->wait[i]->nice){
				min = cond->wait[i]->nice;
				unblock = (void*)cond->wait[i];
				idx = i;
			}
		}

		cond->waitCnt--;
		cond->wait[idx] = 0;

		wakeup(unblock);
	}

	release(&cond->lock);

	return 0;
}
