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
	//打开信号量
	mutex = sem_open(MUTUX_NAME,O_CREAT);
	if (mutex == SEM_FAILED)
	{
		printf("Receiver打开信号量mutex失败\n");
		sem_unlink(MUTUX_NAME);
		perror("错误：");
		exit(0);
	}
	s_done = sem_open(SYNC1_NAME,O_CREAT);
	if (s_done == SEM_FAILED)
	{
		printf("Receiver打开信号量sender_done失败\n");
		sem_unlink(SYNC1_NAME);
		perror("错误：");
		exit(0);
	}	
	r_done = sem_open(SYNC2_NAME,O_CREAT);
	if (r_done == SEM_FAILED)
	{
		printf("Receiver打开信号量receiver_done失败\n");
		sem_unlink(SYNC2_NAME);
		perror("错误：");
		exit(0);
	}	
	
	/*
	函数原型:sem_t  *sem_open(const char* name, int oflag, mode_t mode, int value);
	name:信号量名字
	oflag:标识，O_CREAT表示新建一个，如果已经存在就打开，忽略后两个变量
	mode:访问权限，0644表示用户具有读写权限，组用户和其它用户具有只读权限
	value:信号量的初始值
	*/
	
	//打开共享内存
	int shmid;
	shmid = shmget(key, SIZE, 0666);
	if (shmid < 0)
		{
			printf("Receiver打开共享内存空间失败\n");
			perror("错误：");
			exit(0);
		}
	//将共享内存连接（映射）到本程序的地址空间
	char *shmaddr;
	shmaddr = shmat(shmid, NULL, 0);//返回值是映射到自己空间的地址
	//接收Sender发来的内容
	char *get_message;
	sem_wait(s_done);
	sem_wait(mutex);
	strcpy(get_message, shmaddr);
	sem_post(mutex);
	printf("Receiver接收内容:%s\n",get_message);
	//Receiver发送over
	char *reply = "over";
	sem_wait(mutex);
	strcpy(shmaddr, reply);
	sem_post(mutex);
	//Receiver释放“令牌”
	sem_post(r_done);
	sem_close(mutex);
	sem_close(r_done);
	sem_close(s_done);
	printf("Receiver进程结束\n");
	return 0;
	
}
