#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
	int pid;
	int flag;
	int nice;
	int outnice;
	int outprio;
	printf("请输入进程ID 、 flag 、nice值:\n");
	scanf("%d %d %d",&pid,&flag,&nice);
	syscall(334,pid,flag,nice,&outnice,&outprio);
	if(flag == 1)
		printf("该进程的优先级为:%d ,nice值为:%d",outprio,outnice);
	return 0;

}



