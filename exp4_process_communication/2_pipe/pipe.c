#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>

int main()
{
	pid_t pid;
	sem_t sem;
	int i;
	int flag = -1;
	int fd[2];
	int count = 0;
	char string1[] = "this is zy's pipe!";
	char string2[] = "zy try2!";
	char string3[] = "let's go zy!";
	char readbuffer[100]; // 存储用read()函数从pipe中读出的字符串；
	int real_read, real_write;//存储read()和write()的返回值，返回值为字符长度；
	
	for(i=0;i<100;i++)
	{readbuffer[i] = '\0';}

	sem_init(&sem,0,1); // 0表示信号量类型，全局；1表示sem信号量的最大值为1；
	flag = pipe(fd);
	pid = fork();  // 子进程中fork()返回值为0；父进程中返回创建的子进程的pid;
	if(pid<0)
	{
		printf("fork error!");
	}
	else if(pid == 0)
	{
		printf("child process: %d its father is %d\n",getpid(),getppid());
		close(fd[0]);
		sem_wait(&sem);
		real_write = write(fd[1], string1, strlen(string1));
		if(real_write>0)
		{
			printf("%d characters had wrote in pipe \n",real_write);
			count++;
		}
		sem_post(&sem);
	}
	else
	{
		pid_t pid1;
		pid1 = fork();
		if(pid1<0)
		{
			printf("fork error!");
		}
		else if(pid1 == 0)
		{
			printf("child process: %d its father is %d\n",getpid(),getppid());
			close(fd[0]);
			sem_wait(&sem);
			real_write = write(fd[1], string2, strlen(string2));
			if(real_write>0)
			{
				printf("%d characters had wrote in pipe \n",real_write);
				count++;
			}
			sem_post(&sem);
		}
		else
		{
			pid_t pid2;
			pid2 = fork();
			if(pid2<0)
			{
				printf("fork error!");
			}
			else if(pid2 == 0)
			{
				printf("child process: %d its father is %d\n",getpid(),getppid());
				close(fd[0]);
				sem_wait(&sem);
				real_write = write(fd[1], string3, strlen(string3));
				if(real_write>0)
				{
					printf("%d characters had wrote in pipe \n",real_write);
					count++;
				}
				sem_post(&sem);
			}
			else
			{
				close(fd[1]);
				sleep(3);
				sem_wait(&sem);
				real_read = read(fd[0], readbuffer, sizeof(readbuffer));
				printf("%d characters: %s received \n",real_read,readbuffer);
				sem_post(&sem);
				
				
			
				
			}
		}
	}
}
