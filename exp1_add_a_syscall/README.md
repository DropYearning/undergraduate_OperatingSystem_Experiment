# 实验一 linux内核编译及添加系统调用
## 添加一个系统调用，实现对指定进程的nice 值的修改或读取功能，并返回进程最新的nice 值及优先级prio
建议调用原型为：
`int mysetnice(pid_t pid, int flag, int nicevalue, void __user * prio, void __user * nice);`
***

1. 修改系统调用表(./arch/x86/entyr/syscalls/syscall_64.tbl)

  <系统调用号> <commom/64/x32> <系统调用名> <服务例程入口地址>

2. 申明系统调用服务例程的原型(linux-4.12/include/linux/syscalls.h)

  ![](https://static.notion-static.com/4cb433a9faf44ba3a5332c341862bd2f/Untitled)

3. 实现系统调用服务例程(linux-4.12/kernel/sys.c)

  ![](https://static.notion-static.com/96f77a47fa4f4c63a58145eb4a8aa3fd/Untitled)

4. 重新编译内核
  1. cp linux... /usr/src
  2.  xz -d linux-4.4.19.tar.xz
  3.  tar -xvf linux-4.4.19.tar
  4. (apt-get install libncurses5-dev)
  5. make mrproper
  6. make menuconfig

    ![](https://static.notion-static.com/08bd5aa42cc64a4a97729c0356d89a5a/Untitled)

    ![](https://static.notion-static.com/8fd6280b053f4fe5a823f5e89ad949c2/Untitled)

    ![](https://static.notion-static.com/4dee0f03de3d429a833df0e3aad2b018/Untitled)

    ![](https://static.notion-static.com/b515bc55a49f4f4ebeebcab5c56ce8d9/Untitled)

    ![](https://static.notion-static.com/8951004d87184ed7a43b78dc774440cf/Untitled)

  7. (apt-get install libssl-dev)
  8. make bzImage -j8
  9. make modules
  10. make modules_install
  11. mkinitramfs 4.13.8 -o /boot/initrd-4.13.8.img
  12. make install
  13. update-grub2
  14. reboot

---

### 编写用户测试程序

      #include <linux/unistd.h>
      #include <sys/syscall.h>
      #define __NR_mysyscall 326 /*系统调用号根据实验具体数字而定*/
      int main()
      {
      syscall(__NR_mysyscall); /*或syscall(326) */
      }

  ---

  ### getpriority( )

      // E:\A\linux-4.13.7\kernel\sys.c
      // getpriority()函数的系统调用例程实现代码如下
      
      SYSCALL_DEFINE2(getpriority, int, which, int, who)
       [//which](//which) == PRIO_PROCESS(一个特定进程) | PRIO_PGRP(一个进程组所有的进程)|PRIO_USER(一个用户的所有进程)
      {
      	struct task_struct *g, *p;
      	struct user_struct *user;
      	const struct cred *cred = current_cred();
      	long niceval, retval = -ESRCH;
      	struct pid *pgrp;
      	kuid_t uid;
      
      	if (which > PRIO_USER || which < PRIO_PROCESS)
      		return -EINVAL;
      
      	rcu_read_lock();
      	read_lock(&tasklist_lock);
      	switch (which) {
      	case PRIO_PROCESS:
      		if (who)
      			p = find_task_by_vpid(who);
      		else
      			p = current;
      		if (p) {
      			niceval = nice_to_rlimit(task_nice(p));
      			if (niceval > retval)
      				retval = niceval;
      		}
      		break;
      	case PRIO_PGRP:
      		if (who)
      			pgrp = find_vpid(who);
      		else
      			pgrp = task_pgrp(current);
      		do_each_pid_thread(pgrp, PIDTYPE_PGID, p) {
      			niceval = nice_to_rlimit(task_nice(p));
      			if (niceval > retval)
      				retval = niceval;
      		} while_each_pid_thread(pgrp, PIDTYPE_PGID, p);
      		break;
      	case PRIO_USER:
      		uid = make_kuid(cred->user_ns, who);
      		user = cred->user;
      		if (!who)
      			uid = cred->uid;
      		else if (!uid_eq(uid, cred->uid)) {
      			user = find_user(uid);
      			if (!user)
      				goto out_unlock;	/* No processes for this user */
      		}
      		do_each_thread(g, p) {
      			if (uid_eq(task_uid(p), uid) && task_pid_vnr(p)) {
      				niceval = nice_to_rlimit(task_nice(p));
      				if (niceval > retval)
      					retval = niceval;
      			}
      		} while_each_thread(g, p);
      		if (!uid_eq(uid, cred->uid))
      			free_uid(user);		/* for find_user() */
      		break;
      	}
      out_unlock:
      	read_unlock(&tasklist_lock);
      	rcu_read_unlock();
      
      	return retval;
      }

  ###  find_get_pid( )

      struct pid *find_get_pid(pid_t nr)
      {
      	struct pid *pid;
      
      	rcu_read_lock();
      	pid = get_pid(find_vpid(nr));
      	rcu_read_unlock();
      
      	return pid;
      }

       [//](//a) pid 进程ID
       [//flag](//flag) 0时读 1时改
       [//prio](//prio) \nice 进程当前优先级及nice值

