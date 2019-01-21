// 共享内存相关
#include<sys/ipc.h>
#include<sys/shm.h>
//信号量相关
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <semaphore.h>
//基本头文件
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//错误信息输出
#include <errno.h>

#define SIZE 128 //共享内存空间大小(单位：字节)
char MUTUX_NAME[] = "mutex" ;//自定义有名信号量的名字
char SYNC1_NAME[] = "s_done";//令牌,表示sender已经发送信息
char SYNC2_NAME[] = "r_done";//令牌，表示receiver已经发送信息

int main()
{
	//key 是标识共享内存的自定义键值
	key_t key;
	key = 1114;	
	sem_t *mutex;//信号量mutex用于互斥访问共享内存
	sem_t *s_done;//令牌,表示sender已经发送信息
	sem_t *r_done;//令牌，表示receiver已经发送信息
	//初始化信号量
	mutex = sem_open(MUTUX_NAME,O_CREAT,0644,1);
	if(mutex == SEM_FAILED)
	{
		printf("Sender创建信号量mutex失败\n");
		sem_unlink(MUTUX_NAME);
		perror("错误：");
		exit(0);
	}
	r_done = sem_open(SYNC2_NAME,O_CREAT,0644,0);
	if(r_done == SEM_FAILED)
	{
		printf("Sender创建信号量receiver_done失败\n");
		sem_unlink(SYNC2_NAME);
		perror("错误：");
		exit(0);
	}	
	s_done = sem_open(SYNC1_NAME,O_CREAT,0644,0);
	if(s_done == SEM_FAILED)
	{
		printf("Sender创建信号量sender_done失败\n");
		sem_unlink(SYNC1_NAME);
		perror("错误：");
		exit(0);
	}	

	/*
	函数原型:sem_t  *sem_open(const char* name, int oflag, mode_t mode, int value);
	name:信号量名字
	oflag:标识，O_CREAT表示新建一个，如果已经存在就打开
	mode:访问权限，0644表示用户具有读写权限，组用户和其它用户具有只读权限
	value:信号量的初始值
	*/
	//创建一块共享内存
	int shmid;
	shmid = shmget(key, SIZE, IPC_CREAT|0666 );
	if (shmid < 0)
		{
			printf("Sender创建共享内存空间失败\n");
			perror("错误：");
			exit(0);
		}
	//将共享内存连接（映射）到本程序的地址空间
	char *shmaddr;
	shmaddr = shmat(shmid, NULL, 0);//返回值是映射到自己空间的地址
	/*
	If shmaddr is NULL, the system chooses a suitable (unused) page-
          aligned address to attach the segment.
	*/
	/*
	 If SHM_RDONLY is specified in shmflg, the segment is attached for read-
       ing and the process must have read permission for the segment.   Other-
       wise  the  segment  is attached for read and write and the process must
       have read and write permission for the segment.
	*/
	if (shmaddr < 0)
		{
			printf("Sender映射共享内存空间失败\n");
			perror("错误：");
			exit(0);
		}	
	printf("请输入Sender发送的字符串：\n");
	char message[SIZE];
	scanf("%s", message);
	//Sender向Receiver发送内容
	sem_wait(mutex);
	strcpy(shmaddr, message);
	sem_post(mutex);
	sem_post(s_done);
	printf("Sender发送内容:%s\n",message);
	//等待receiver应答
	char reply[SIZE];
	sem_wait(r_done);
	strcpy(reply, shmaddr);
	printf("Sender接收内容:%s\n",reply);
	//删除信号量
	sem_close(mutex);
	sem_unlink(MUTUX_NAME);
	sem_close(s_done);
	sem_unlink(SYNC1_NAME);
	sem_close(r_done);
	sem_unlink(SYNC2_NAME);	
	//删除共享内存
	shmctl(shmid, IPC_RMID, 0);
	printf("Sender进程结束\n");

	return 0;

}
