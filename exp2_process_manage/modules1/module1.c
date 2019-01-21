#include <linux/init.h>   
#include <linux/module.h>   
#include <linux/kernel.h>  
#include <linux/sched.h> 
//#include <linux/init_task.h>
static int module1_init(void)
{
	struct task_struct *p;
	for_each_process(p)
	{
		if(p->mm == NULL)
		{
			int nice = task_nice(p);
			printk(KERN_ALERT"进程名:%s\t进程号:%d\t进程状态:%ld\t进程nice值:%d\n",p->comm,p->pid,p->state,nice);
		}
	}
	return 0;
}

static void module1_exit(void)
{
	printk(KERN_ALERT"卸载模块\n");
}

module_init(module1_init);
module_exit(module1_exit);
MODULE_LICENSE("GPL");
