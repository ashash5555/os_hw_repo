#include <linux/kernel.h>
#include <linux/sched.h>
#include "sched/sched.h"
#include <linux/errno.h>

asmlinkage long sys_hello(void) {
	printk("Hello, World!");
	return 0;
}

asmlinkage long get_vruntime(void) {
	// declaring params before starting the function
	struct sched_entity* se;
	long vruntime_ns;
	
	// 'current' is the current tasks PCB
	se = &(current->se);

	// inside 'se' we can find the virtual time
	vruntime_ns = se->vruntime;
	
	return vruntime_ns;
}


asmlinkage long increment_vruntime(long delta) {
	struct sched_entity* se;
	unsigned long long vruntime_ns;
	struct rq* rq;
	struct task_struct* p;

	if (delta < 0) {
		return -EINVAL;
	}

	p = current;
	rq = task_rq(p);
	se = &(p->se);
	
	deactivate_task(rq, current, DEQUEUE_NOCLOCK);

	vruntime_ns = se->vruntime;
	vruntime_ns = vruntime_ns + delta;

	se->vruntime = vruntime_ns;

	activate_task(rq, current, ENQUEUE_NOCLOCK);

	// resched_curr(rq);

	return 0;
}
