//系统调用相关 fork() execl()
#include <unistd.h>
//wait()相关
#include <sys/types.h>
#include <sys/wait.h>
//基本头文件
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//报错相关
#include <errno.h>

//所有可识别的命令字符串
char *commands [4] = {"exit","cmd1","cmd2","cmd3"};

//找到这条命令的下标
int find_command(char *cmd)
{
    int i;
    for(i=0;i<4;i++)
    {
        if(strcmp(cmd,commands[i])==0)
        {
            return i;
        }
    }
    return -1;
}

//调用对应的子进程
//fork（）若成功调用一次则返回两个值，子进程返回0，父进程返回子进程pid
//引用一位网友的话来解释fork函数返回的值为什么在父子进程中不同。“其实就相当于链表，进程形成了链表，父进程的fork函数返回的值指向子进程的进程id, 因为子进程没有子进程，所以其fork函数返回的值为0
//子进程是父进程的副本，它将获得父进程数据空间、堆、栈等资源的副本。注意，子进程持有的是上述存储空间的“副本”，这意味着父子进程间不共享这些存储空间。
//例如：在shell命令行执行ps命令，实际上是shell进程调用fork复制一个新的子进程，在利用exec系统调用将新产生的子进程完全替换成ps进程。

void call_new_process(int cmdIndex)
{
    pid_t pid;
    if((pid = fork())<0)
    {
        perror("创建子进程失败:");
        exit(0);
    }
	//父子进程都从此处开始执行
    else if(pid == 0)//若返回值是0,表示这是子进程
    {
        int result ;
        switch(cmdIndex)
        {
            
	//The  exec() functions return only if an error has occurred.  
	//The returnvalue is -1, and errno is set to indicate the error.
	/*
	函数原型： int execl(const char *path, const char *arg, ...)
	path:调用程序的执行路径
	arg:一般第一个参数为要执行命令名
	NULL:NULL用在最后一个参数后表示结束
	作用：The exec() family of functions replaces the current process image with a new process image.
	*/
			case 1:
                {
					result = execl("./cmd1","cmd1",NULL);
					break;
				}
                
            case 2:
                {
					result = execl("./cmd2","cmd2",NULL);
					break;
				}
            case 3:
                {
					result = execl("./cmd3","cmd3",NULL);
					break;
				}
        }
        if(result == -1)
        {
			perror("调用程序错误：");
            exit(0);
        }
		//下面的代码不会被输出。因为若成功，则子程序已经被替换成cmd程序，下方的语句不再存在
		else
		{		
        	printf("调用程序成功\n");
       		exit(0);
		}
    }
	//如果是父进程，运行下方else块中的语句
    else
    {
        return;
    }
}

//根据命令决定下一步做什么
void judge_command(int cmdIndex)
{
    switch(cmdIndex)
    {
        case -1:
            printf("找不到这条命令，请重输！\n");
            return;
        case 0:
			printf("已退出！\n");
            exit(0);
        default:
            call_new_process(cmdIndex);
            return;
    }
}

//主函数
int main()
{
    char command[20];
    int index;
    while(1)
    {
        printf("请输入命令$ ");
        scanf("%s",command);
        index = find_command(command);
        judge_command(index);
        wait(0);//父进程等待自己的子进程完成
	//父进程一旦调用了wait就立即阻塞自己，由wait自动分析是否当前进程的某个子进程已经退出，如果让它找到了这样一个已经变成僵尸的子进程，wait就会收集这个子进程的信息，并把它彻底销毁后返回；如果没有找到这样一个子进程，wait就会一直阻塞在这里，直到有一个出现为止。
    }
}
