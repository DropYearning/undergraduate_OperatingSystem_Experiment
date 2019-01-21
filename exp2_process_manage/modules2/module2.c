#include <linux/init.h>   
#include <linux/module.h>   
#include <linux/kernel.h>  
#include <linux/sched.h> 
#include <linux/init_task.h>
#include <linux/moduleparam.h>
#include <linux/list.h>  

static int pid;
module_param(pid,int,0644);

static int module2_init(void)
{
	struct task_struct *task,*temp;
	struct list_head *h;
	//task = find_task_by_vpid(pid);
	task = pid_task(find_vpid(pid), PIDTYPE_PID);
	if( task != NULL)
	{
		printk(KERN_ALERT"显示%d号进程%s的家族信息如下:\n", task->pid, task->comm);
		printk(KERN_ALERT"该进程的父进程为:%d号进程%s\n", task->parent->pid, task->parent->comm);
		list_for_each(h, &task->parent->children)
		{	
			temp = list_entry(h, struct task_struct,sibling);
			if(temp != task)
			printk(KERN_ALERT"该进程的兄弟进程为:%d号进程%s\n", temp->pid,temp->comm);
		}
		list_for_each(h, &task->children)
		{	
			temp = list_entry(h, struct task_struct,sibling);
			printk(KERN_ALERT"该进程的子进程为:%d号进程%s\n", temp->pid,temp->comm);
		}	
	}
	else
	printk(KERN_ALERT"找不到该进程！\n");
	return 0;
}

static void module2_exit(void)
{
	printk(KERN_ALERT"卸载模块\n");
}




module_init(module2_init);
module_exit(module2_exit);
MODULE_LICENSE("GPL");
