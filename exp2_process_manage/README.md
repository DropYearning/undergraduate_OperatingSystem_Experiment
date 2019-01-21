# 实验二 linux 内核模块编程
## (1)设计一个模块，要求列出系统中所有内核线程的程序名、PID 号、进程状态及进程优先级。
## (2)设计一个带参数的模块，其参数为某个进程的PID 号，该模块的功能是列出该进程的家族信息，包括父进程、兄弟进程和子进程的程序名、PID 号
*** 
## (1) volatile(不稳定的) long state

volatile关键字是一种类型修饰符，用它声明的类型变量表示可以被某些编译器未知的因素更改，比如：操作系统、硬件或者其它线程等。遇到这个关键字声明的变量，编译器对访问该变量的代码就不再进行优化，从而可以提供对特殊地址的稳定访问。

告诉编译器每次都需要读取值,而不是寄存器中的副本.更加小心地访问和更改变量

## E:\A\linux-4.13.7\include\linux\sched.h

    /* Used in tsk->state: */
    #define TASK_RUNNING			0
    #define TASK_INTERRUPTIBLE		1
    #define TASK_UNINTERRUPTIBLE		2
    #define __TASK_STOPPED			4
    #define __TASK_TRACED			8
    /* Used in tsk->exit_state: */
    #define EXIT_DEAD			16
    #define EXIT_ZOMBIE			32
    #define EXIT_TRACE			(EXIT_ZOMBIE | EXIT_DEAD)
    /* Used in tsk->state again: */
    #define TASK_DEAD			64
    #define TASK_WAKEKILL			128
    #define TASK_WAKING			256
    #define TASK_PARKED			512
    #define TASK_NOLOAD			1024
    #define TASK_NEW			2048
    #define TASK_STATE_MAX			4096

 **(2) char comm[TASK_COMM_LEN]; ** 

/* Task command name length: */
#define TASK_COMM_LEN	16

 **(3)for_each_process** 

#define for_each_process(p)
for (p = &init_task ; (p = next_task(p)) != &init_task ; )

"双向循环链表"list_head,并且头结点是不使用的

next_task -> list_entry_rcu

rcu: Read-copy update (RCU) 是一种 2002 年 10 月被引入到内核当中的同步机制。通过允许在更新的同时读数据，RCU 提高了同步机制的可伸缩性（scalability）。

list_head链表中不保存数据,只保存成员变量的地址

访问到它所有者节点的数据呢？linux提供了list_entry这个宏

 **(10)temp = list_entry(h, struct task_struct,sibling);** 

list_entry(ptr, type, member)
#define container_of(ptr, type, member)
(type *)((char *)(ptr) - (char *) &((type *)0)->member)

#define list_entry(ptr, type, member)
container_of(ptr, type, member)

 **(5)set_user_nice** 

    //set_user_nice
    void set_user_nice(struct task_struct *p, long nice)
    {
    
    if (task_has_dl_policy(p) || task_has_rt_policy(p)) {
    	p->static_prio = NICE_TO_PRIO(nice);
    	goto out_unlock;
    }
    
    ...

 **(7)task_nice(p)** 

    static inline int task_nice(const struct task_struct *p)
    {
    	return PRIO_TO_NICE((p)->static_prio);
    }

    #define NICE_TO_PRIO(nice)	((nice) + DEFAULT_PRIO)
    #define PRIO_TO_NICE(prio)	((prio) - DEFAULT_PRIO)
    
    #define DEFAULT_PRIO		(MAX_RT_PRIO + NICE_WIDTH / 2)
    #define MAX_NICE	19
    #define MIN_NICE	-20
    #define NICE_WIDTH	(MAX_NICE - MIN_NICE + 1)
    

 **(8)** 

 **task = pid_task(find_vpid(pid), PIDTYPE_PID);** 

task = find_task_by_vpid(pid);

    struct task_struct *find_task_by_vpid(pid_t vnr)
    {
    	return find_task_by_pid_ns(vnr, task_active_pid_ns(current));
    }

    struct task_struct *find_task_by_pid_ns(pid_t nr, struct pid_namespace *ns)
    {
    	RCU_LOCKDEP_WARN(!rcu_read_lock_held(),
    			 "find_task_by_pid_ns() needs rcu_read_lock() protection");
    	return pid_task(find_pid_ns(nr, ns), PIDTYPE_PID);
    }

    struct pid *find_pid_ns(int nr, struct pid_namespace *ns)
    {
    	struct upid *pnr;
    
    	hlist_for_each_entry_rcu(pnr,
    			&pid_hash[pid_hashfn(nr, ns)], pid_chain)
    		if (pnr->nr == nr && pnr->ns == ns)
    			return container_of(pnr, struct pid,
    					numbers[ns->level]);
    
    	return NULL;
    }

 **(9)list_for_each(h, &task->sibling)** 

    #define list_for_each(pos, head) \
     for (pos = (head)->next, prefetch(pos->next); pos != (head); \
     pos = pos->next, prefetch(pos->next))

 **(11)if(p->mm == NULL)** 
内核线程没有用户地址空间


