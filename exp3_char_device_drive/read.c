#include <fcntl.h>  	//O_RDWR
#include <stdio.h>  	//printf()
#include <string.h>	//strlen()
#include <unistd.h>	//系统调用
#include <sys/ioctl.h>	//ioctl

int main()  
{  
	int fd; 
	char outbuf[512] = {0};  
	fd = open("/dev/module4",O_RDWR);
	if(fd<0)  
	{  
		printf("打开文件失败！ \n");  
	}  
	else
		printf("打开文件成功！ \n"); 
	read(fd,outbuf,512);  
	printf("从设备文件中读取的数据为：%s \n",outbuf);  
	close(fd);  
	return 0;
}  
