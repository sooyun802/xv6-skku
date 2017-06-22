struct mutex_t {
	int flag;
	int tid;
	int id;
	struct proc *wait[7];
	int waitCnt;
	struct spinlock lock;
};

struct cond_t {
	int id;
	struct proc *wait[7];
	int waitCnt;
	struct spinlock lock;
};
