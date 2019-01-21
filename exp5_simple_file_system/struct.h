#ifndef STRUCT_H
#define STRUCT_H

#include<stdio.h>
#include<malloc.h>
#include<time.h>
#include<stdlib.h>
#include<string.h>

#define BLOCKSIZE 1024		//磁盘块大小（字节）
#define SIZE 1024000		//虚拟磁盘空间大小
#define END 65535			//FAT中文件结束标志
#define FREE 0				//FAT中文件盘块空闲标志
#define ROOT_BLOCKNUM 2		//根目录所占盘块数
#define MAX_OPEN_FILE 10	//最大同时打开文件数目
#define MAX_TXT_SIZE	100000	//最大文件长度


//FCB数据结构
typedef struct FCB
{
	char filename[8];   		// 8B文件名
	char exname[3];				// 3B扩展名
	unsigned char attribute;	// 1B属性,值为0表示目录文件，值为1表示数据文件
	unsigned short time;		// 2B文件创建时间
	unsigned short date;		// 2B文件创建日期
	unsigned short first;		// 2B首块号
	unsigned long length;		// 4B文件长度
	char free; 		/*表示目录项是否为空，值为0空,值为1分配*/
}fcb;


//FAT表数据结构（仿照FAT16），每个表项16位
//一共有1000块，FAT表有1000项，需要2000字节，即每个FAT需要两个盘块保存
//每个FAT表项2直接，一个FAT盘块可以保存 512 个盘块的FAT表项
typedef struct FAT
{
	unsigned short id; //id字段2字节，16位,表示下一个盘块号
}fat;

//USEROPNE表，用户打开文件表,
typedef struct USEROPEN
{
	char filename[8];   			/*8B文件名*/
	char exname[3];					/*3B扩展名*/
	unsigned char attribute;		/*文件属性字段*/
	unsigned short time;			/*文件创建时间*/
	unsigned short date;			/*文件创建日期*/
	unsigned short first;			/*首块号*/
	unsigned long length;			/*文件大小*/
	char free; 		/*表示打开文件表项是否为空，值为0空,值为1分配*/
	/****************上面为对应文件FCB的内容*********************/

	int father;		// 父目录在打开文件表项的位置

	int dirno;		/*相应打开文件的目录项在父目录文件中的盘块号*/
	int diroff;		/*相应打开文件的目录项在父目录文件的dirno盘块中的目录项序号*/
	//change?
	char dir[MAX_OPEN_FILE][80];	/*相应打开文件所在的目录*/
	int count;			/*读写指针在文件中的位置*/
	char fcbstate;		/*是否修改了文件的FCB内容，修改置1，否则为0*/
	char topenfile;		/*表示该用户打开的表项是否为空，若值为0，表示为空,否则表示已被某打开文件占据*/
}useropen;


//引导块
typedef struct BLOCK0
{
	//change?改魔数名字?改魔数大小？
	unsigned short fbnum;		//文件系统魔数
	char information[200];		//存储一些描述信息
	unsigned short root;		// 根目录文件的起始盘块号
	unsigned char *startblock;	// 虚拟磁盘上数据区开始位置
}block0;


unsigned char *myvhard;		/*指向虚拟磁盘的起始地址*/
useropen openfilelist[MAX_OPEN_FILE];	/*用户打开文件表数组*/
useropen *ptrcurdir;		/*指向用户打开文件表中的当前目录所在打开文件表项的位置*/
//change？
int curfd; 	//当前打开文件在打开文件表中的位子å

char currentdir[80];			/*记录当前目录的目录名，包含目录的路径*/
unsigned char *startp;			/*记录虚拟磁盘上数据区开始位置*/
char FILENAME[]="myfilesys";   	/*临时文件名*/

unsigned char buffer[SIZE]; 	/*临时缓冲区，与临时文件的大小相当*/

#endif
