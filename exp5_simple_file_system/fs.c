#include "struct.h"


/*函数声明*/
void startsys();
void my_format();
void my_cd(char *dirname);
void my_mkdir(char *dirname);
void my_rmdir(char *dirname);
void my_ls();
void my_create(char *filename);
void my_rm(char *filename);
int my_open(char *filename);
int my_close(int fd);
int my_write(int fd);
int do_write(int fd,char *text,int len,char wstyle);
unsigned short findFree();
int my_read(int fd,int len);
int do_read(int fd,int len,char *text);
void my_exitsys();


/*文件系统初始化*/
void startsys()
{
	FILE *fp;
	int i;
	//malloc如果分配成功则返回指向被分配内存的指针
	//新的ANSIC标准规定，该函数返回为void型指针，因此必要时要进行类型转换
	myvhard=(unsigned char *)malloc(SIZE); //申请一块内存空间用于虚拟文件系统
	//malloc函数分配得到的内存空间是未初始化的,需要用memset来将其初始化为全0
	memset(myvhard,0,SIZE);

	//以读模式打开临时文件
	fp=fopen(FILENAME,"r");
	if(fp) //fp != NULL，表示打开临时文件成功
	{
		/**
		 * size_t fread ( void *buffer, size_t size, size_t count, FILE *stream) ;
		 * @param buffer 用于接收数据的内存地址
		 * @param SIZE   要读的每个数据项的字节数
		 * @param count  要读count个数据项，每个数据项size个字节.
		 * @param fp     打开文件指针，指示要读取的文件
		 */
		fread(buffer,SIZE,1,fp); //从临时文件中读出大小为SIZE字节的二进制数据到buffer中
		fclose(fp); //读入buffer后直接关闭即可

		// 将虚拟磁盘第一个块作为引导块，开始的 8 个字节是文件系统的魔数，记为 “10101010”,即0xaa
		// change？改成8个字节?
		if(buffer[0]!=0xaa||buffer[1]!=0xaa) //若不是魔数开头
		{
			printf("临时文件不存在，重新创建临时文件.\n");
			my_format();
		}
		else
		{
			for(i=0;i<SIZE;i++)
					myvhard[i]=buffer[i];//将buffer中的内容（即临时文件内容）拷贝到新申请的内存空间myvhard中
		}
	}
	else //若打开临时文件失败，重新创建临时文件
	{
		printf("临时文件不存在，重新创建临时文件.\n");
		my_format();
	}

	//初始化用户打开文件表
	//将打开文件表的表项[0]分配给根目录文件
	strcpy(openfilelist[0].filename,"root");
	strcpy(openfilelist[0].exname,"di");
	openfilelist[0].attribute=0x20;

	//根目录位置在第6个块的开始处，从根目录文件的FCB处拷贝根目录信息到打开文件表
	openfilelist[0].time=((fcb *)(myvhard+5*BLOCKSIZE))->time;
	openfilelist[0].date=((fcb *)(myvhard+5*BLOCKSIZE))->date;
	openfilelist[0].first=((fcb *)(myvhard+5*BLOCKSIZE))->first;
	openfilelist[0].length=((fcb *)(myvhard+5*BLOCKSIZE))->length;
	openfilelist[0].free=1;

	//由于根目录没有上级目录，所以表项中的 dirno 和 diroff 分别置为 5（根目录所 在起始块号）和 0
	openfilelist[0].dirno=5;
	openfilelist[0].diroff=0;

	openfilelist[0].count=0;
	openfilelist[0].fcbstate=0;
	//change？
	openfilelist[0].topenfile=0;

	openfilelist[0].father=0;
	memset(currentdir,0,sizeof(currentdir));
	//当前目录名为root,注意转义
	strcpy(currentdir,"\\root\\");
	strcpy(openfilelist->dir[0],currentdir);

	startp=((block0 *)myvhard)->startblock;
	ptrcurdir=&openfilelist[0];
	curfd=0;
}

//对虚拟磁盘进行格式化，布局虚拟磁盘，建立根目录文件
void my_format()
{
	FILE *fp;
	fat *fat1,*fat2;
	block0 *b0;
	time_t *now;
	//tm是time.h头文件中的结构体
	struct tm *nowtime;
	unsigned char *p;
	fcb *root;
	int i;

	//初始化b0,fat1,fat2在内存中对应的空间中的起始地址
	p=myvhard;
	b0=(block0 *)p;
	fat1=(fat *)(p+BLOCKSIZE);
	fat2=(fat *)(p+3*BLOCKSIZE);

	//初始化引导块
	b0->fbnum=0xaaaa;	//引导块的有一个short字段的魔数
	b0->root = 5;  // 根目录文件的起始盘块号
	strcpy(b0->information,"简单文件系统 \n Blocksize=1KB Virturl size=1000KB Blocknum=1000 RootBlocknum=2\n");
	b0->startblock=p+BLOCKSIZE*4;

	//前5个块为已分配
	fat1->id=END;
	fat2->id=END;
	fat1++;fat2++;

	fat1->id=END;
	fat2->id=END;
	fat1++;fat2++;

	fat1->id=END;
	fat2->id=END;
	fat1++;fat2++;

	fat1->id=END;
	fat2->id=END;
	fat1++;fat2++;

	fat1->id=END;
	fat2->id=END;
	fat1++;fat2++;

	//FAT表的第6、7两块用作根目录区（固定根目录区大小=2块）
	//change?
	fat1->id=6;
	fat2->id=6;
	fat1++;fat2++;

	fat1->id=END;
	fat2->id=END;
	fat1++;fat2++;

	//后面993块为空闲数据块
	for(i=7;i<SIZE/BLOCKSIZE;i++)
	{
		(*fat1).id=FREE;
		(*fat2).id=FREE;
		fat1++;
		fat2++;
	}

	/*
	创建根目录文件root，将数据区的第一块分配给根目录区
	在给磁盘上创建两个特殊的目录项：".",".."，
	除了文件名之外，其它都相同
	*/

	//p指向第六块开始处,即根目录开始处
	p+=BLOCKSIZE*5;
	//初始化根目录文件的FCB
	root=(fcb *)p;
	strcpy(root->filename,".");
	strcpy(root->exname,"di");
	root->attribute=0x20;//0010 1000
	now=(time_t *)malloc(sizeof(time_t));
	time(now);
	nowtime=localtime(now);
	root->time=nowtime->tm_hour*2048+nowtime->tm_min*32+nowtime->tm_sec/2;
	root->date=(nowtime->tm_year-80)*512+(nowtime->tm_mon+1)*32+nowtime->tm_mday;
	root->first=5;
	//???
	root->length=2*sizeof(fcb);

	root++;
	strcpy(root->filename,"..");
	strcpy(root->exname,"di");
	root->attribute=0x20;//0010 1000
	time(now);
	nowtime=localtime(now);
	root->time=nowtime->tm_hour*2048+nowtime->tm_min*32+nowtime->tm_sec/2;
	root->date=(nowtime->tm_year-80)*512+(nowtime->tm_mon+1)*32+nowtime->tm_mday;
	root->first=5;
	root->length=2*sizeof(fcb);
	root++;

	//剩下的都是根目录项
	for(i=2;i<BLOCKSIZE*2/sizeof(fcb);i++,root++)
	{
		root->filename[0]='\0';
	}

	fp=fopen(filename,"w");
	fwrite(myvhard,SIZE,1,fp);
	free(now);
	fclose(fp);

}

//退出文件系统
void my_exitsys()
{
	FILE *fp;
	fcb *rootfcb;
	char text[];
	while(curfd)
		curfd=my_close(curfd);
	if(openfilelist[curfd].fcbstate)
	{
		openfilelist[curfd].count=0;
		do_read(curfd,openfilelist[curfd].length,text);
		rootfcb=(char *)text;
		rootfcb->length=openfilelist[curfd].length;
		openfilelist[curfd].count=0;
		do_write(curfd,text,openfilelist[curfd].length,2);
	}
	fp=fopen(filename,"w");
	fwrite(myvhard,SIZE,1,fp);
	free(myvhard);
	fclose(fp);
}

//实际读文件函数
int do_read(int fd,int len,char *text)
{
	unsigned char *buf;
    unsigned short bknum;
	int off,i,lentmp;
	unsigned char *bkptr;
	char *txtmp,*p;
	//fat1表示fat表位置
	//fatptr表示当前块在fat表中的位置
	fat *fat1,*fatptr;
    fat1=(fat *)(myvhard+BLOCKSIZE);
	lentmp = len;
	//change? txtmp多余
	txtmp=text;
	/*
	申请1024B空间作为缓冲区buffer
	*/
	buf=(unsigned char *)malloc(1024);
	if(buf==NULL)
	{
		printf("错误，分配空间失败!\n");
		return -1;
	}

	//找到从哪里开始读，off即现在读到哪了
	off = openfilelist[fd].count;
	bknum = openfilelist[fd].first;
	fatptr = fat1+bknum;
	while(off >= BLOCKSIZE)
	{
		off=off-BLOCKSIZE;
		bknum=fatptr->id;
		fatptr=fat1+bknum;
		if(bknum == END)
		{
			printf("错误，该盘块不存在!\n");
			return -1;
		}
	}//最后的bknum就是当前要读的位置所在的盘块号,off就是读的位置在这一块中的偏移量

	bkptr=(unsigned char *)(myvhard+bknum*BLOCKSIZE);//bkptr指向当前偏移量所在盘块在内存中的位置
	//将这一块中的所有内容读到buf中
	for(i=0;i<BLOCKSIZE;i++)
	{
		buf[i]=bkptr[i];
	}
	while(len > 0)
	{
		if(BLOCKSIZE-off > len)//要读的数据没有超出当前盘块的范围时
		{
			for(p=buf+off;len>0;p++,txtmp++)//将内容读出到txtmp中
			{
				*txtmp=*p;
				len--;
				off++;
				openfilelist[fd].count++;
			}
		}
		else//当要读的数据超过当前盘块的范围时
		{

			for(p=buf+off;p<buf+BLOCKSIZE;p++,txtmp++)//先把这一块能读的都读完
			{
				*txtmp=*p;
				len--;
				openfilelist[fd].count++;
			}
			//在读fatptr中指向的下一块,即这个文件的下一个盘块
			off=0;
			bknum =fatptr->id;
			fatptr = fat1+bknum;
			bkptr=(unsigned char *)(myvhard+bknum*BLOCKSIZE);
			//此处有循环,会回到上面的部分继续操作
			for(i=0;i<BLOCKSIZE;i++)
			{
				buf[i]=bkptr[i];
			}
		}
	}
	free(buf);
	return lentmp-len;
}

//读文件函数
int my_read(int fd,int len)
{
	char text[];
	if(fd > MAX_OPEN_FILE)
	{
		printf("错误，该文件不存在!\n");
		return -1;
	}
	openfilelist[curfd].count=0;
	if(do_read(fd,len,text)>0)
	{
		text[len]='\0';
		printf("%s\n",text);
	}
	else
	{
		printf("错误，无法读取该文件!\n");
		return -1;
	}
	return len;
}


//实际写文件函数
int do_write(int fd,char *text,int len,char wstyle)
{
	unsigned char *buf;
	unsigned short bknum;
	int off,tmplen=0,tmplen2=0,i,rwptr;
	unsigned char *bkptr,*p;
	char *txtmp,flag=0;
	fat *fat1,*fatptr;
    fat1=(fat *)(myvhard+BLOCKSIZE);
	txtmp=text;
	/*
	申请临时1024B的buffer
	*/
	buf=(unsigned char *)malloc(BLOCKSIZE);
	if(buf==NULL)
	{
		printf("错误，分配空间失败!\n");
		return -1;
	}

	rwptr = off = openfilelist[fd].count;
	bknum = openfilelist[fd].first;
	fatptr=fat1+bknum;
	while(off >= BLOCKSIZE )
	{
		off=off-BLOCKSIZE;
		bknum=fatptr->id;
		if(bknum == END)
		{
			bknum=fatptr->id=findFree();
			if(bknum==END) return -1;
			fatptr=fat1+bknum;
			fatptr->id=END;
		}
		fatptr=fat1+bknum;
	}

	fatptr->id=END;
	bkptr=(unsigned char *)(myvhard+bknum*BLOCKSIZE);
	while(tmplen<len)
	{
			for(i=0;i<BLOCKSIZE;i++)
			{
				buf[i]=0;
			}
			for(i=0;i<BLOCKSIZE;i++)
			{
					buf[i]=bkptr[i];
					tmplen2++;
					if(tmplen2==openfilelist[curfd].length)
						break;
			}

		for(p=buf+off;p<buf+BLOCKSIZE;p++)
		{
			*p=*txtmp;
			tmplen++;
			txtmp++;
			off++;
			if(tmplen==len)
				break;
		}

		for(i=0;i<BLOCKSIZE;i++)
		{
				bkptr[i]=buf[i];
		}
		openfilelist[fd].count=rwptr+tmplen;
		if(off>=BLOCKSIZE)
		{
			off=off-BLOCKSIZE;
			bknum=fatptr->id;
			if(bknum==END)
			{
				bknum=fatptr->id=findFree();
				if(bknum==END) return -1;
				fatptr=fat1+bknum;
				fatptr->id=END;
			}
			fatptr=fat1+bknum;
			bkptr=(unsigned char *)(myvhard+bknum*BLOCKSIZE);
		}
	}
	free(buf);
	if(openfilelist[fd].count>openfilelist[fd].length)
	{
		openfilelist[fd].length=openfilelist[fd].count;
	}
	openfilelist[fd].fcbstate=1;
	return tmplen;
}

//写文件函数
int my_write(int fd)
{
	int wstyle=0,wlen=0;
	fat *fat1,*fatptr;
	unsigned short bknum;
	unsigned char *bkptr;
	char text[];
	fat1=(fat *)(myvhard+BLOCKSIZE);
	if(fd>MAX_OPEN_FILE)
	{
		printf("错误,该文件不存在.\n");
		return -1;
	}
	while(wstyle<1||wstyle>3)
	{
		printf("请输入选择的写方式序号:\n1.截断写\t2.覆盖写\t3.追加写\n");
		scanf("%d",&wstyle);
		getchar();
		switch(wstyle)
		{
		case 1://截断写
			{
				bknum=openfilelist[fd].first;
				//释放除了first块的其他所有快
				fatptr=fat1+bknum;
				while(fatptr->id!=END)
				{
					bknum=fatptr->id;
					fatptr->id=FREE;
					fatptr=fat1+bknum;
				}
				fatptr->id=FREE;
				//回到first块
				bknum=openfilelist[fd].first;
				fatptr=fat1+bknum;
				fatptr->id=END;
				//重置之前写入的数据
				openfilelist[fd].length=0;
				openfilelist[fd].count=0;
				break;
			}
		case 2://覆盖写
			{
				//???
				openfilelist[fd].count=0;
				break;
			}
		case 3://追加写
			{
				bknum=openfilelist[fd].first;
				fatptr=fat1+bknum;
				openfilelist[fd].count=0;
				//找到最后的那个盘块
				while(fatptr->id!=END)
				{
					bknum=fatptr->id;
					fatptr=fat1+bknum;
					openfilelist[fd].count+=BLOCKSIZE;
				}
				bkptr=(unsigned char *)(myvhard+bknum*BLOCKSIZE);
				while(*bkptr !=0)
				{
					bkptr++;
					openfilelist[fd].count++;
				}
				break;
			}
		default:
			break;
		}
	}
	printf("请输入要写入的数据:\n");
	gets(text);
	if(do_write(fd,text,strlen(text),wstyle)>0)
	{
		wlen+=strlen(text);
	}
	else
	{
		return -1;
	}

	if(openfilelist[fd].count>openfilelist[fd].length)
	{
		openfilelist[fd].length=openfilelist[fd].count;
	}
	openfilelist[fd].fcbstate=1;
	return wlen;
}


//寻找下一个空闲盘块,返回值就是找到的空闲盘块号
unsigned short findFree()
{
	unsigned short i;
	fat *fat1,*fatptr;

	fat1=(fat *)(myvhard+BLOCKSIZE);
	//change i=7?
	for(i=6; i<END; i++)
	{
		fatptr=fat1+i;//fatptr一开始指向第七块的开始处
		if(fatptr->id == FREE)
		{
			return i;
		}
	}
	printf("错误，已经没有空闲的盘块!\n");
	return END;
}

//寻找空闲文件表项
int findFreeO()
{
	int i;
	for(i=0;i<MAX_OPEN_FILE;i++)
	{
		if(openfilelist[i].free==0)
		{
			return i;
		}
	}
	printf("错误，打开文件的数量超过10!\n");
	return -1;
}

//创建子目录函数,在当前目录下创建名为dirname的目录
void my_mkdir(char *dirname)
{
	fcb *dirfcb,*pcbtmp;
	int rbn,i,fd;
	unsigned short bknum;
	char text[],*p;
	time_t *now;
	struct tm *nowtime;
	/*
	将当前的文件信息读到text中
	rbn 是实际读取的字节数
	*/
	openfilelist[curfd].count=0;
	rbn = do_read(curfd,openfilelist[curfd].length,text);
	dirfcb=(fcb *)text;
	/*
	检测是否有相同的目录名
	*/
	for(i=0;i<rbn/sizeof(fcb);i++)
	{
		if(strcmp(dirname,dirfcb->filename)==0)
		{
			printf("错误，该目录名已存在.\n");
			return -1;
		}
		dirfcb++;
	}

	//可能有之前删除目录后空出来的目录项位置
	dirfcb=(fcb *)text;
	for(i=0;i<rbn/sizeof(fcb);i++)
	{
		if(strcmp(dirfcb->filename,"")==0)
			break;
		dirfcb++;
	}
	openfilelist[curfd].count=i*sizeof(fcb);

	/*
		分配一个空闲的打开文件表项
	*/
	fd=findFreeO();
	if(fd<0)
	{
		return -1;
	}

	//为新创建的目录文件寻找一个空闲盘块
	bknum = findFree();
	if(bknum == END )
	{
		return -1;
	}

	//创建一个临时PCB空间
	pcbtmp=(fcb *)malloc(sizeof(fcb));
	now=(time_t *)malloc(sizeof(time_t));

	//在当前目录下新建目录项
	pcbtmp->attribute=0x20;//0011 0000 子目录
	time(now);
	nowtime=localtime(now);
	pcbtmp->time=nowtime->tm_hour*2048+nowtime->tm_min*32+nowtime->tm_sec/2;
	pcbtmp->date=(nowtime->tm_year-80)*512+(nowtime->tm_mon+1)*32+nowtime->tm_mday;
	strcpy(pcbtmp->filename,dirname);
	strcpy(pcbtmp->exname,"di");
	pcbtmp->first=bknum;
	pcbtmp->length=2*sizeof(fcb);

	openfilelist[fd].attribute=pcbtmp->attribute;
	openfilelist[fd].count=0;
	openfilelist[fd].date=pcbtmp->date;
	strcpy(openfilelist[fd].dir[fd],openfilelist[curfd].dir[curfd]);

	p=openfilelist[fd].dir[fd];
	while(*p!='\0')
		p++;
	strcpy(p,dirname);
	while(*p!='\0') p++;
	*p='\\';p++;
	*p='\0';

	openfilelist[fd].dirno=openfilelist[curfd].first;
	openfilelist[fd].diroff=i;
	strcpy(openfilelist[fd].exname,pcbtmp->exname);
	strcpy(openfilelist[fd].filename,pcbtmp->filename);
	openfilelist[fd].fcbstate=1;
	openfilelist[fd].first=pcbtmp->first;
	openfilelist[fd].length=pcbtmp->length;
	openfilelist[fd].free=1;
	openfilelist[fd].time=pcbtmp->time;
	openfilelist[fd].topenfile=1;

	do_write(curfd,(char *)pcbtmp,sizeof(fcb),2);

	pcbtmp->attribute=0x20;//00101000
	time(now);
	nowtime=localtime(now);
	pcbtmp->time=nowtime->tm_hour*2048+nowtime->tm_min*32+nowtime->tm_sec/2;
	pcbtmp->date=(nowtime->tm_year-80)*512+(nowtime->tm_mon+1)*32+nowtime->tm_mday;
	strcpy(pcbtmp->filename,".");
	strcpy(pcbtmp->exname,"di");
	pcbtmp->first=bknum;
	pcbtmp->length=2*sizeof(fcb);

	do_write(fd,(char *)pcbtmp,sizeof(fcb),2);

	pcbtmp->attribute=0x20;
	time(now);
	nowtime=localtime(now);
	pcbtmp->time=nowtime->tm_hour*2048+nowtime->tm_min*32+nowtime->tm_sec/2;
	pcbtmp->date=(nowtime->tm_year-80)*512+(nowtime->tm_mon+1)*32+nowtime->tm_mday;
	strcpy(pcbtmp->filename,"..");
	strcpy(pcbtmp->exname,"di");
	pcbtmp->first=openfilelist[curfd].first;
	pcbtmp->length=openfilelist[curfd].length;

	do_write(fd,(char *)pcbtmp,sizeof(fcb),2);

	openfilelist[curfd].count=0;
	do_read(curfd,openfilelist[curfd].length,text);

	pcbtmp=(fcb *)text;
	pcbtmp->length=openfilelist[curfd].length;
	my_close(fd);

	openfilelist[curfd].count=0;
	do_write(curfd,text,pcbtmp->length,2);
}

//显示目录函数
void my_ls()
{
	fcb *fcbptr;
	int i;
	char text[];
	unsigned short bknum;
	openfilelist[curfd].count=0;
	do_read(curfd,openfilelist[curfd].length,text);
	fcbptr=(fcb *)text;
	for(i=0;i<(int)(openfilelist[curfd].length/sizeof(fcb));i++)
	{
		if(fcbptr->filename[0]!='\0')
		{
			if(fcbptr->attribute&0x20)
			{
				printf("%s\\\t\t<DIR>\t\t%d/%d/%d\t%02d:%02d:%02d\n",fcbptr->filename,((fcbptr->date)>>9)+1980,((fcbptr->date)>>5)&0x000f,(fcbptr->date)&0x001f,fcbptr->time>>11,(fcbptr->time>>5)&0x003f,fcbptr->time&0x001f*2);
			}
			else
			{
				printf("%s.%s\t\t%dB\t\t%d/%d/%d\t%02d:%02d:%02d\t\n",fcbptr->filename,fcbptr->exname,fcbptr->length,((fcbptr->date)>>9)+1980,(fcbptr->date>>5)&0x000f,fcbptr->date&0x1f,fcbptr->time>>11,(fcbptr->time>>5)&0x3f,fcbptr->time&0x1f*2);
			}
		}
		fcbptr++;
	}
	openfilelist[curfd].count=0;
}

//删除子目录函数
void my_rmdir(char *dirname)
{
	int rbn,fd;
	char text[];
	fcb *fcbptr,*fcbtmp,*fcbtmp2;
	unsigned short bknum;
	int i,j;
	fat *fat1,*fatptr;

	if(strcmp(dirname,".")==0 || strcmp(dirname,"..")==0)
	{
		printf("错误，该目录不能删除.\n");
		return -1;
	}
	fat1=(fat *)(myvhard+BLOCKSIZE);
	openfilelist[curfd].count=0;
	rbn=do_read(curfd,openfilelist[curfd].length,text);
	fcbptr=(fcb *)text;
	for(i=0;i<rbn/sizeof(fcb);i++)
	{
		if(strcmp(dirname,fcbptr->filename)==0)
		{
			break;
		}
		fcbptr++;
	}
	if(i >= rbn/sizeof(fcb))
	{
		printf("错误，该目录不存在.\n");
		return -1;
	}

	bknum=fcbptr->first;
	//fcbtmp2动指针,检查当前目录文件的子目录是否为空
	fcbtmp2=fcbtmp=(fcb *)(myvhard+bknum*BLOCKSIZE);
	for(j=0;j<fcbtmp->length/sizeof(fcb);j++)
	{
		if(strcmp(fcbtmp2->filename,".") && strcmp(fcbtmp2->filename,"..") && fcbtmp2->filename[0]!='\0')
		{
			printf("错误，子目录非空.\n");
			return -1;
		}
		fcbtmp2++;
	}

	while(bknum!=END)
	{
		fatptr=fat1+bknum;
		bknum=fatptr->id;
		fatptr->id=FREE;
	}

	strcpy(fcbptr->filename,"");
	strcpy(fcbptr->exname,"");
	fcbptr->first=END;
	openfilelist[curfd].count=0;
	do_write(curfd,text,openfilelist[curfd].length,2);

}

//更改目录函数
void my_cd(char *dirname)
{
	char *p,text[];
	int fd,i;
	p=strtok(dirname,"\\");
	if(strcmp(p,".")==0)//当前目录
		return;
	if(strcmp(p,"..")==0)//上级目录
	{
		fd=openfilelist[curfd].father;
		my_close(curfd);
		curfd=fd;
		ptrcurdir=&openfilelist[curfd];
		return;
	}
	if(strcmp(p,"root")==0 || strcmp(p,"~")==0)//直接回到根目录
	{
		for(i=1;i<MAX_OPEN_FILE;i++)
		{
			if(openfilelist[i].free)
				my_close(i);
		}
		ptrcurdir=&openfilelist[0];
		curfd=0;
		p=strtok(NULL,"\\");
	}
	while(p)
	{
		fd=my_open(p);
		if(fd>0)
		{
			ptrcurdir=&openfilelist[fd];
			curfd=fd;
		}
		else
			return -1;
		p=strtok(NULL,"\\");
	}
}

//打开文件函数
int my_open(char *filename)
{
	int i,fd,rbn;
	char text[],*p,*fname,*exname;
	fcb *fcbptr;
	char exnamed=0;
	fname=strtok(filename,".");
	exname=strtok(NULL,".");
	if(!exname)
	{
		exname=(char *)malloc(3);
		strcpy(exname,"di");
		exnamed=1;
	}
	for(i=0;i<MAX_OPEN_FILE;i++)
	{
		if(strcmp(openfilelist[i].filename,filename)==0 &&strcmp(openfilelist[i].exname,exname)==0&& i!=curfd)
		{
			printf("错误，文件已经打开.\n");
			return -1;
		}
	}
	openfilelist[curfd].count=0;
	rbn=do_read(curfd,openfilelist[curfd].length,text);
	fcbptr=(fcb *)text;

	for(i=0;i<rbn/sizeof(fcb);i++)
	{
		if(strcmp(filename,fcbptr->filename)==0 && strcmp(fcbptr->exname,exname)==0)
		{
				break;
		}
		fcbptr++;
	}
	if(i>=rbn/sizeof(fcb))
	{
		printf("错误，文件不存在.\n");
		return curfd;
	}

	fd=findFreeO();
	if(fd==-1)
	{
		return -1;
	}

	strcpy(openfilelist[fd].filename,fcbptr->filename);
	strcpy(openfilelist[fd].exname,fcbptr->exname);
	openfilelist[fd].attribute=fcbptr->attribute;
	openfilelist[fd].count=0;
	openfilelist[fd].date=fcbptr->date;
	openfilelist[fd].first=fcbptr->first;
	openfilelist[fd].length=fcbptr->length;
	openfilelist[fd].time=fcbptr->time;
	openfilelist[fd].father=curfd;
	openfilelist[fd].dirno=openfilelist[curfd].first;
	openfilelist[fd].diroff=i;
	openfilelist[fd].fcbstate=0;
	openfilelist[fd].free=1;
	openfilelist[fd].topenfile=1;
	strcpy(openfilelist[fd].dir[fd],openfilelist[curfd].dir[curfd]);
	p=openfilelist[fd].dir[fd];
	while(*p!='\0')
		p++;
	strcpy(p,filename);
	while(*p!='\0') p++;
	if(openfilelist[fd].attribute&0x20)
	{
		*p='\\';p++;	*p='\0';

	}
	else
	{
		*p='.';p++;
		strcpy(p,openfilelist[fd].exname);
	}

	//释放开辟的空间
	if(exnamed)
	{
		free(exname);
	}

	return fd;
}

//关闭文件函数
int my_close(int fd)
{
	fcb *fafcb;
	char text[];
	int fa;

	//检查fd有效性
	if(fd > MAX_OPEN_FILE || fd<=0)
	{
		printf("错误，文件不存在.\n");
		return -1;
	}

	//fa是当前目录在当前目录文件中的序号
	fa=openfilelist[fd].father;
	if(openfilelist[fd].fcbstate)
	{
		fa=openfilelist[fd].father;
		openfilelist[fa].count=0;
		do_read(fa,openfilelist[fa].length,text);
		//找到要关闭的那个fcb在目录中的位置,并更新为最新的信息
		fafcb=(fcb *)(text+sizeof(fcb)*openfilelist[fd].diroff);
		fafcb->attribute=openfilelist[fd].attribute;
		fafcb->date=openfilelist[fd].date;
		fafcb->first=openfilelist[fd].first;
		fafcb->length=openfilelist[fd].length;
		fafcb->time=openfilelist[fd].time;
		strcpy(fafcb->filename,openfilelist[fd].filename);
		strcpy(fafcb->exname,openfilelist[fd].exname);
		openfilelist[fa].count=0;
		do_write(fa,text,openfilelist[fa].length,2);
	}
	//清空当前打开文件的打开文件表项
	openfilelist[fd].attribute=0;
	openfilelist[fd].count=0;
	openfilelist[fd].date=0;
	strcpy(openfilelist[fd].dir[fd],"");
	strcpy(openfilelist[fd].filename,"");
	strcpy(openfilelist[fd].exname,"");
	openfilelist[fd].length=0;
	openfilelist[fd].time=0;
	openfilelist[fd].free=0;
	openfilelist[fd].topenfile=0;

	return fa;
}


//创建文件函数
void my_create(char *filename)
{
	char *fname,*exname,text[];
	int fd,rbn,i;
	fcb *filefcb,*fcbtmp;
	time_t *now;
	struct tm *nowtime;
	unsigned short bknum;
	fat *fat1,*fatptr;
	fat1=(fat *)(myvhard+BLOCKSIZE);
	fname=strtok(filename,".");
	exname=strtok(NULL,".");
	if(strcmp(fname,"")==0)
	{
		printf("错误，请输入要创建文件的文件名.\n");
		return -1;
	}
	if(!exname)
	{
		printf("错误，创建的文件必须要有一个扩展名.\n");
		return -1;
	}
	//初始化读写指针为0,用于后面的do_read
	openfilelist[curfd].count=0;
	//读出fd文件从读写指针开始的长度为length的内容到text中，返回rbn = 实际读出的字节数(read byte number)
	//读出fd目录文件的所有内容(其实就是目录文件中的所有目录项)
	rbn=do_read(curfd,openfilelist[curfd].length,text);
	//filefcb用来将fd目录文件一个个切分成当前目录的所有fcb
	filefcb=(fcb *)text;
	// 依次遍历当前目录所有文件的fcb，看是否存在重名
	for(i=0;i<rbn/sizeof(fcb);i++)
	{
		if(strcmp(fname,filefcb->filename)==0 && strcmp(exname,filefcb->exname)==0)
		{
			printf("错误，文件名已存在.\n");
			return -1;
		}
		filefcb++;//filefcb每次加一个fcb的长度
	}

	//如果有之前rm释放掉的fcb空间,则找到它,并以之为写入位置;如果没有空位就是接下去的一块空闲空间
	filefcb=(fcb *)text;
	for(i=0;i<rbn/sizeof(fcb);i++)
	{
		if(strcmp(filefcb->filename,"")==0)
			break;
		filefcb++;
	}
	openfilelist[curfd].count=i*sizeof(fcb);

	//bknum是找到的下一个空闲块的块号
	bknum=findFree();
	if(bknum==END)
	{
		return -1;
	}

	//创建一块新fcb空间，并填入相关信息
	fcbtmp=(fcb *)malloc(sizeof(fcb));
	now=(time_t *)malloc(sizeof(time_t));
	fcbtmp->attribute=0x00;//=0 普通文件
	time(now);
	nowtime=localtime(now);
	fcbtmp->time=nowtime->tm_hour*2048+nowtime->tm_min*32+nowtime->tm_sec/2;
	fcbtmp->date=(nowtime->tm_year-80)*512+(nowtime->tm_mon+1)*32+nowtime->tm_mday;
	strcpy(fcbtmp->filename,fname);
	strcpy(fcbtmp->exname,exname);
	fcbtmp->first=bknum;
	fcbtmp->length=0;

	//把新创建的fcb内容写入当前目录文件
	do_write(curfd,(char *)fcbtmp,sizeof(fcb),2);//=2表示覆盖写,即从当前读写指针开始写(即之前找到的空位)
	free(fcbtmp);
	free(now);

	//把修改后的目录文件全部读到text中
	openfilelist[curfd].count=0;
	do_read(curfd,openfilelist[curfd].length,text);
	//之前修改的是目录文件的数据部分,接下来修改目录文件的FCB部分的length
	fcbtmp=(fcb *)text;
	fcbtmp->length=openfilelist[curfd].length;

	openfilelist[curfd].count=0;
	do_write(curfd,text,openfilelist[curfd].length,2);
	openfilelist[curfd].fcbstate=1;

	//修改用掉的空闲块为END(相当于把这个空闲盘块分给新创建的文件了)
	fatptr=(fat *)(fat1+bknum);
	fatptr->id=END;
}

//删除文件函数
void my_rm(char *filename)
{
	char *fname,*exname;
	char text[];
	fcb *fcbptr;
	int i,rbn;
	unsigned short bknum;
	fat *fat1,*fatptr;

	fat1=(fat *)(myvhard+BLOCKSIZE);
	fname=strtok(filename,".");
	exname=strtok(NULL,".");
	if(!fname || strcmp(fname,"")==0)
	{
		printf("错误，请输入要删除文件的文件名.\n");
		return -1;
	}
	if(!exname)
	{
		printf("错误，请输入要删除文件的扩展名.\n");
		return -1;
	}
	openfilelist[curfd].count=0;
	rbn=do_read(curfd,openfilelist[curfd].length,text);
	fcbptr=(fcb *)text;
	for(i=0;i<rbn/sizeof(fcb);i++)
	{
		if(strcmp(fname,fcbptr->filename)==0 && strcmp(exname,fcbptr->exname)==0)
		{
			break;
		}
		fcbptr++;
	}
	if(i>=rbn/sizeof(fcb))
	{
		printf("错误，文件不存在.\n");
		return -1;
	}

	bknum=fcbptr->first;
	while(bknum!=END)
	{
		fatptr=fat1+bknum;
		bknum=fatptr->id;
		fatptr->id=FREE;
	}

	strcpy(fcbptr->filename,"");
	strcpy(fcbptr->exname,"");
	fcbptr->first=END;
	openfilelist[curfd].count=0;
	do_write(curfd,text,openfilelist[curfd].length,2);
}

/*主函数*/
void main()
{
	char cmd[15][10]={"mkdir","rmdir","ls","cd","create","rm","open","close","write","read","exit"};
	char s[30],*sp;
	int cmdn,i;
	startsys();
	printf("*********************简单文件系统 第1组********************************\n\n");
	printf("命令名\t\t命令参数\t\t命令说明\n\n");
	printf("cd\t\t目录名(路径名)\t\t切换当前目录到指定目录\n");
	printf("mkdir\t\t目录名\t\t\t在当前目录创建新目录\n");
	printf("rmdir\t\t目录名\t\t\t在当前目录删除指定目录\n");
	printf("ls\t\t无\t\t\t显示当前目录下的目录和文件\n");
	printf("create\t\t文件名\t\t\t在当前目录下创建指定文件\n");
	printf("rm\t\t文件名\t\t\t在当前目录下删除指定文件\n");
	printf("open\t\t文件名\t\t\t在当前目录下打开指定文件\n");
	printf("write\t\t无\t\t\t在打开文件状态下，写该文件\n");
	printf("read\t\t无\t\t\t在打开文件状态下，读取该文件\n");
	printf("close\t\t无\t\t\t在打开文件状态下，读取该文件\n");
	printf("exit\t\t无\t\t\t退出系统\n\n");
	printf("*********************************************************************\n\n");
	while(1)
	{
		printf("%s>",openfilelist[curfd].dir[curfd]);
		gets(s);
		cmdn=-1;
		if(strcmp(s,""))
		{
			//首次调用时，s指向要分解的字符串，之后再次调用要把s设成NULL
			sp=strtok(s," ");
			for(i=0;i<15;i++)
			{
				if(strcmp(sp,cmd[i])==0)
				{
					cmdn=i;
					break;
				}
			}
			switch(cmdn)
			{
			case 0://mkdir
				sp=strtok(NULL," ");
				//change是否可以去掉attribute
				if(sp && openfilelist[curfd].attribute&0x20)
					my_mkdir(sp);
				else
					printf("请输入正确的命令！\n");
				break;
			case 1://rmdir
				sp=strtok(NULL," ");
				if(sp && openfilelist[curfd].attribute&0x20)
					my_rmdir(sp);
				else
					printf("请输入正确的命令！\n");
				break;
			case 2://ls
				if(openfilelist[curfd].attribute&0x20)
				{
					my_ls();
					putchar('\n');
				}
				else
					printf("请输入正确的命令！\n");
				break;
			case 3://cd
				sp=strtok(NULL," ");
				if(sp && openfilelist[curfd].attribute&0x20)
					my_cd(sp);
				else
					printf("请输入正确的命令！\n");
				break;
			case 4://create
				sp=strtok(NULL," ");
				if(sp && openfilelist[curfd].attribute&0x20)
					my_create(sp);
				else
					printf("请输入正确的命令！\n");
				break;
			case 5://rm
				sp=strtok(NULL," ");
				if(sp && openfilelist[curfd].attribute&0x20)
					my_rm(sp);
				else
					printf("请输入正确的命令！\n");
				break;
			case 6://open
				sp=strtok(NULL," ");
				if(sp && openfilelist[curfd].attribute&0x20)
				{
					if(strchr(sp,'.'))
						curfd=my_open(sp);
					else
						printf("创建的文件必须要有一个扩展名！\n");
				}
				else
					printf("请输入正确的命令！\n");
				break;
			case 7://close
				if(!(openfilelist[curfd].attribute&0x20))
					curfd=my_close(curfd);
				else
					printf("没有打开的文件！\n");
				break;
			case 8://write
				if(!(openfilelist[curfd].attribute&0x20))
					my_write(curfd);
				else
					printf("没有打开的文件！\n");
				break;
			case 9://read
				if(!(openfilelist[curfd].attribute&0x20))
					my_read(curfd,openfilelist[curfd].length);
				else
					printf("没有打开的文件！\n");
				break;
			case 10://exit
				if(openfilelist[curfd].attribute&0x20)
				{
					my_exitsys();
					return ;
				}
				else
					printf("请输入正确的命令！\n");
				break;
			default:
				printf("请输入正确的命令！\n");
				break;
			}
		}
	}
}
