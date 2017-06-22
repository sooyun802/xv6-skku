#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

extern struct _ptable {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;
extern struct proc *initproc;
extern void forkret(void);
extern void trapret(void);

int thread_create(void *(*function)(void *), int priority, void *arg, void *stack){
	struct proc *nt;
	char *sp;
	int tid, i;

	if(*proc->tcnt == 8){
		return -1;
	}

	acquire(&ptable.lock);

	for(nt = ptable.proc; nt < &ptable.proc[NPROC]; nt++)
		if(nt->state == UNUSED)
			goto found;

	release(&ptable.lock);
	return -1;

found:
	nt->state = EMBRYO;
	nt->pid = proc->pid;
	nt->tcnt = proc->tcnt;
	nt->tid = nt->pid + (*nt->tcnt)++;

	release(&ptable.lock);

	if((nt->kstack = kalloc()) == 0){
		nt->state = UNUSED;
		return -1;
	}
	sp = nt->kstack + KSTACKSIZE;

	sp -= sizeof *nt->tf;
	nt->tf = (struct trapframe*)sp;
	*nt->tf = *proc->tf;

	sp -= 4;
	*(uint*)sp = (uint)trapret;

	sp -= sizeof *nt->context;
	nt->context = (struct context*)sp;
	memset(nt->context, 0, sizeof *nt->context);
	nt->context->eip = (uint)forkret;

	nt->pgdir = proc->pgdir;

	nt->sz = proc->sz;
	nt->parent = proc->parent;
	nt->nice = priority;

	nt->tf->eax = 0;

	nt->tf->eip = (int)function;
	nt->tf->esp = (int)(stack + PGSIZE - 8);

	*(int*)(nt->tf->esp + 4) = (int)arg;
	*(int*)(nt->tf->esp) = 0xffffffff;
	nt->stack = stack;

	for(i = 0; i < NOFILE; i++){
		if(proc->ofile[i])
			nt->ofile[i] = proc->ofile[i];
	}
	nt->cwd = proc->cwd;

	safestrcpy(nt->name, proc->name, sizeof(proc->name));

	tid = nt->tid;

	acquire(&ptable.lock);

	nt->state = RUNNABLE;
	proc->state = RUNNABLE;
	addqueue(nt);
	addqueue(proc);
	sched();

	release(&ptable.lock);

	return tid;
}

void thread_exit(void *retval){
	struct proc *t;
	int i;

	if(proc == initproc)
		panic("init exiting");

	if(proc->tid == proc->pid){
		exit();
	}
	else{
		for(t = ptable.proc; t < &ptable.proc[NPROC]; t++){
			if(t->pid == proc->pid && t->state == RUNNABLE){
				for(i = 0; i < NOFILE; i++){
					if(t->tid != t->pid && proc->ofile[i])
						t->ofile[i] = proc->ofile[i];
					else if(t->tid == t->pid && proc->ofile[i] && !t->ofile[i])
						t->ofile[i] = filedup(proc->ofile[i]);
				}
			}
		}

		proc->cwd = 0;

		acquire(&ptable.lock);

		for(t = ptable.proc; t < &ptable.proc[NPROC]; t++){
			if(t->pid == proc->pid)
				wakeup1(t);
		}

		*(int*)(proc->stack + PGSIZE - 8) = (int)retval;

		proc->state = ZOMBIE;
		deletequeue(proc);
		sched();
		panic("zombie exit");
	}
}

int thread_join(int tid, void **retval){
	struct proc *t;
	int havekids;

	acquire(&ptable.lock);

	for(;;){
		havekids = 0;
		for(t = ptable.proc; t < &ptable.proc[NPROC]; t++){
			if(t->pid != proc->pid || t->tid != tid)
				continue;
			havekids = 1;
			if(t->state == ZOMBIE){
				*(int*)retval = *(int*)(t->stack + PGSIZE - 8);
				kfree(t->kstack);
				t->kstack = 0;
				if(t->pid == t->tid)
					freevm(t->pgdir);
				if(t->pid != t->tid){
					(*t->tcnt)--;
					t->tcnt = 0;
				}
				t->pid = 0;
				t->tid = 0;
				t->nice = -1;
				t->parent = 0;
				t->name[0] = 0;
				t->killed = 0;
				t->state = UNUSED;

				release(&ptable.lock);

				return 0;
			}
		}

		if(!havekids || proc->killed){
			release(&ptable.lock);
			return -1;
		}

		sleep(proc, &ptable.lock);
	}

	return -1;
}

int gettid(void){
	// return -1;
	return proc->tid;
}
