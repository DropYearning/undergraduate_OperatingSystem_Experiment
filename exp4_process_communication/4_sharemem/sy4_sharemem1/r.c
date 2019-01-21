// 共享内存相关
#include<sys/ipc.h>
#include<sys/shm.h>
//信号量相关
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <semaphore.h>
//基本头文件
#include<stdio.h>
#include<stdlib.h>
#include <string.h>

#define SIZE 128 //共享内存空间大小(单位：字节)
char MUTUX_NAME[] = "shm_mutex" ;//自定义有名信号量的名字
char SYNC_NAME[] = "shm_sync";//同步信号量

int main()
{
	//key 是标识共享内存的自定义键值
	key_t key;
	key = 1113;	
	sem_t *mutex;//信号量mutex用于互斥访问共享内存
	sem_t *sync; //信号量sync用户实现两个进程的同步
	//打开信号量
	mutex = sem_open(MUTUX_NAME,O_CREAT);
	if (mutex == SEM_FAILED)
	{
		printf("Receiver打开信号量mutex失败");
		exit(0);
	}
	sync = sem_open(SYNC_NAME,O_CREAT);
	if (sync == SEM_FAILED)
	{
		printf("Receiver打开信号量sync失败");
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
			printf("Receiver打开共享内存空间失败");
			exit(0);
		}
	//将共享内存连接（映射）到本程序的地址空间
	char *shmaddr;
	shmaddr = shmat(shmid, NULL, 0);//返回值是映射到自己空间的地址
	//接收Sender发来的内容
	char *get_message;
	sem_wait(mutex);
	strcpy(get_message, shmaddr);
	sem_post(mutex);
	printf("Receiver接收内容:%s",get_message);
	//Receiver发送over
	char *reply = "over";
	sem_wait(mutex);
	strcpy(shmaddr, reply);
	sem_post(mutex);
	//Receiver释放“令牌”
	sem_post(sync);
	sem_close(mutex);
	sem_close(sync);
	printf("Receiver进程结束");
	return 0;
	
}
