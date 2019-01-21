//消息队列相关
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
//信号量相关
#include <semaphore.h>
//多线程相关
#include <pthread.h>
//基本头文件
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h> //S_IRUSR|S_IWUSR Permits the file's owner to read it.Permits the file's owner to write to it.

#define SIZE 128
#define KEY 2014
//自定义消息结构体
struct msgbuf
{
	long mtype;
	char mtext[SIZE];
};

//初始化信号量
sem_t full;//标识现在队列中有消息
sem_t empty;//标识现在消息队列为空
sem_t mutex;//对消息队列互斥访问
sem_t send_over;//标识receiver进程发送了over

pthread_t sender_tid;
pthread_t receiver_tid;

int msgid;//消息队列标识数
//具体实现sender进程
void * sender(void *arg)
{
	char text[SIZE];
	struct msgbuf msg;
	msg.mtype = 1;
	int lenth;
	lenth = sizeof(struct msgbuf) - sizeof(long);
	while (1)
	{
		sem_wait(&empty);//P(empty)
		sem_wait(&mutex);//P(mutex)
		printf("Sender线程:请输入你要发送的字符串:");
		scanf("%s", text);	
		if (strcmp(text, "exit") == 0)//如果发送的是exit,则sender发送end
		{
			strncpy(msg.mtext,"end",SIZE);
			msgsnd(msgid,&msg,lenth,0);
			printf("Sender线程:发送字符串 %s\n",msg.mtext);
			sem_post(&mutex);
			sem_post(&full);
			break;
		}
		strncpy(msg.mtext,text,SIZE);
		if(msgsnd(msgid,&msg,lenth,0) < 0)
			perror("错误：");
		printf("Sender线程:发送字符串 %s\n",msg.mtext);
		sem_post(&full);
		sem_post(&mutex);
	}
	sem_wait(&send_over);
	sem_wait(&mutex);
	sem_wait(&full);
	msgrcv(msgid,&msg,lenth,1,0);
	printf("Sender线程:接收字符串 %s\n",msg.mtext);
	sem_post(&mutex);
	sem_post(&empty);
	//删除消息队列
	if(msgctl (msgid,IPC_RMID,0) == -1)	
	{
		printf("Sender线程:删除消息队列失败,退出\n");
		exit(0);
	}
	pthread_exit(0);
}



//具体实现receiver
void * receiver(void *arg)
{
	struct msgbuf msg;
	msg.mtype = 1;
	int lenth;
	lenth = sizeof(struct msgbuf) - sizeof(long);
	while (1)
	{
		sem_wait(&full);
		sem_wait(&mutex);
		if(msgrcv(msgid,&msg,lenth,1,0) < 0)
			perror("错误：");
		if(strcmp(msg.mtext, "end") == 0) //如果收到的是end,receiver发送over
		{
			strncpy(msg.mtext,"over",SIZE);
			printf("Receiver线程:接收字符串 %s\n",msg.mtext);
			msgsnd(msgid,&msg,lenth,0);
			sem_post(&send_over);
			sem_post(&full);
			sem_post(&mutex);
			break;
		}
		printf("Receiver线程:接收字符串 %s\n",msg.mtext);
		sem_post(&empty);
		sem_post(&mutex);
	}
	pthread_exit(0);
}


int main()
{


/*
	函数原型：int sem_init(sem_t *sem, int pshared, unsigned int value);
	pshared == 0 表示该信号量用于本进程的线程之间
	pshared != 0 表示该信号量用于不同进程之间，且该信号量必须放在这些不同进程的共享空间中
	value 信号量的初始值
*/	

	sem_init(&full,0,0);	
	sem_init(&empty,0,1);	
	sem_init(&mutex,0,1);
	sem_init(&send_over,0,0);	
	//创建消息队列
	msgid = msgget(KEY,S_IRUSR|S_IWUSR|IPC_CREAT);
	if (msgid == -1)
		{
			perror("错误：");
		}
	//创建线程

	if(pthread_create(&sender_tid,NULL,sender,NULL) < 0)
	{
		printf("创建线程失败\n");
		exit(0);
	}
	if(pthread_create(&receiver_tid,NULL,receiver,NULL) < 0)
	{
		printf("创建线程失败\n");
		exit(0);
	}
/*	
int pthread_create(pthread_t *thread, pthread_addr_t *arr,void* (*start_routine)(void *), void *arg);

thread　　　　：用于返回创建的线程的ID
arr　　　  　　  : 用于指定的被创建的线程的属性，上面的函数中使用NULL，表示使用默认的属性
start_routine      : 这是一个函数指针，指向线程被创建后要调用的函数
arg　　　　　   : 用于给线程传递参数，在本例中没有传递参数，所以使用了NULL
*/

	//等待线程结束
	pthread_join(sender_tid,NULL);
	pthread_join(receiver_tid,NULL);
	
	//删除信号量
	sem_destroy(&full);
	sem_destroy(&empty);
	sem_destroy(&mutex);
	sem_destroy(&send_over);
	printf("主进程结束\n")	;
	return 0;
}


