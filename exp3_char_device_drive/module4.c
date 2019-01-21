#include <linux/init.h>   //模块必备    
#include <linux/module.h> //模块必备  
#include <linux/fs.h>     //file_operation, file, inode  
#include <linux/types.h>  //设备号相关, MAJOR, MINOR, MKDEV  
#include <linux/cdev.h>   //字符设备相关操作，init，add，del  
#include <linux/slab.h>   //kmalloc, kfree  
//include <asm/uaccess.h>
#include <linux/uaccess.h>  //copy_to_user, copy_from_user 
#include <linux/kernel.h> //printk, KERN_ALERT
#define SIZE 512

static int major = 271;  
static int minor = 0;  
static dev_t devnum;  

struct mymem_dev
{
	int used;//保存设备文件中已经写入的字节长度
	struct cdev cdev;
	unsigned char mem[SIZE];
};

static struct mymem_dev mycdev;
 
static int module4_open (struct inode *inode, struct file *filp)  
{  
	struct mymem_dev *t;
	t = container_of(inode->i_cdev,struct mymem_dev,cdev);
	filp->private_data = t;
	filp->f_pos = mycdev.used;
	printk("设备文件打开成功！ \n");  
    	return 0;  
}  

static ssize_t module4_read(struct file *file, char __user *buff, size_t lenth, loff_t *offp)
/*
 用户态调用接口： read(fd,buf,len);
 file  ：进行读取信息的目标文件，
 buff  ：对应放置信息的缓冲区（即用户空间内存地址,要把信息读到哪里去）；
 size  ：要读取的信息长度；
 offp  ：读的位置相对于文件开头的偏移，在读取信息后，这个指针一般都会移动，
                 移动的值为要读取信息的长度值
*/
{
	struct mymem_dev *temp = file->private_data;
	char *kbuf;
	kbuf = temp->mem;
	if(copy_to_user(buff,kbuf,lenth) != 0)
	{
		printk("从内核态读取数据失败！\n");
	}
	else	
		printk("设备文件读取 %s 成功！ \n",kbuf);  
    	return lenth;  
}

ssize_t module4_write(struct file *file, const char __user *buff, size_t lenth, loff_t *offp)
/*
 用户态调用接口： write(fd,buf,len);
 filp   ：目标文件结构体指针；
 buff   ：为要写入文件的信息缓冲区；
 lenth  ：为要写入信息的长度；
 offp   ：为当前的偏移位置，这个值通常是用来判断写文件是否越界
*/
{	
	struct mymem_dev *temp = file->private_data;
	char *kbuf;
	kbuf = temp->mem + *offp;
	if(lenth > SIZE - temp->used)
	{
		printk("要写入的字节数越界！\n");
	}
	if(copy_from_user(kbuf,buff,lenth) != 0)
	{
		printk("从用户态写入数据失败！\n");
	}
	else
		printk("设备文件写入 %s 成功！ \n",kbuf);  
	*offp += lenth;    	
	(mycdev.used) = (mycdev.used) + lenth;
	return lenth;  
}


static long module4_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct mymem_dev *temp = filp->private_data;
	switch(cmd)
    	{
        	case 10086:
            		printk("清空内核空间\n");
            		memset(temp->mem, 0, sizeof(temp->mem));
            		break;

        	case 10087:
            		printk("主设备号为：%d \n",major);
            		break;
        	case 10088:
            		printk("次设备号为：%d \n",minor);
            		break;
        	default:
            		return -EFAULT;
    }
}

static int module4_release (struct inode *node, struct file *file)
{
	printk("设备文件关闭成功！\n");
	return 0;
}


static struct file_operations module4_ops=  
{  
	.owner = THIS_MODULE,
    	.open = module4_open,
	.read = module4_read,
	.write = module4_write,
	.release = module4_release,
	.unlocked_ioctl = module4_ioctl,
};  
  
static int module4_init(void)  
{  
    int ret;      
    devnum = MKDEV(major,minor);
 	//静态分配设备号
    ret = register_chrdev_region(devnum,1, "module4_reg");  
    if(ret < 0)  
    {  
        printk("注册设备号失败！ \n");  
        return ret;  
    }  
	//初始化cdev
    cdev_init(&mycdev.cdev,&module4_ops);  
	//向系统添加cdev
    ret = cdev_add(&mycdev.cdev,devnum,1);  
    if(ret < 0)  
    {  
        printk("添加驱动设备失败！ \n");   
    }     
    return 0;  
}  

static void module4_exit(void)  
{  
    cdev_del(&mycdev.cdev);  
    unregister_chrdev_region(devnum,1);  
    printk("卸载模块！（上一次测试执行到这里）！！！！！！！！！！！！！ \n");  
}  

MODULE_LICENSE("GPL");  
module_init(module4_init);  
module_exit(module4_exit);  
