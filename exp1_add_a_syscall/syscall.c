SYSCALL_DEFINE5(mysetnice, pid_t, pid, int, flag, int, nicevalue, void __user *,prio, void __user *, nice)
{
	struct task_struct *task;
	int oldnice;
	task = find_task_by_vpid(pid);
	oldnice = task_nice(task);
	if(flag == 1)
	{
		if(nicevalue >  19 || nicevalue < -20)
		{
			printk("��������ȷ��niceֵ[-20,19]");
			return -EFAULT;
		}
		else
		{
			set_user_nice(task, nicevalue);
			printk("�޸�ǰ��niceֵΪ��%d\t �޸�ǰ��prioֵΪ: %d\n", oldnice, 80+oldnice);
			printk("�޸ĺ��niceֵΪ��%d\t �޸ĺ��prioֵΪ: %d\n", nicevalue, 80+nicevalue);
			return 0;
		}
	}
	else if (flag == 0)
	{
		printk("�ý��̵�niceֵΪ%d\t �ý��̵�prioֵΪ:%d\n", oldnice, 80+oldnice);
		int t_prio;
		t_prio = task->static_prio;		
		copy_to_user(nice,&oldnice,sizeof(oldnice));
		copy_to_user(prio,&t_prio,sizeof(t_prio));
		return 0;
	}
	return -EFAULT;

}