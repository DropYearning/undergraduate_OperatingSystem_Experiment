# 实验三 字符设备驱动设计
## 编写一个字符设备驱动程序，要求实现对该字符设备的打开、读、写、I/O 控制和 关闭 5 个基本操作。为了避免牵涉到汇编语言，这个字符设备并非一个真实的字符设备，而 是用一段内存空间来模拟的。以模块方式加载该驱动程序
## 编写一个应用程序，测试（1）中实现的驱动程序的正确性
***
![](https://static.notion-static.com/a8460373426946cdbd3b377701a8aca3/20160311221053374.png)

![](https://static.notion-static.com/0cfb22c3e00f41b4bb15431af54f0e66/20160311221115875.png)

![](https://static.notion-static.com/d09f78ec17464d3a951334ee28667e0a/20160311221203266.png)

![](https://static.notion-static.com/a5230324fe3349be852dca8977c14aee/20160310210056150.png)

![](https://static.notion-static.com/97ad4355ec454ee3a770bef48300e6cc/20160310213435506.gif)

## (1)struct file *filp (include/linux/fs.h)

    struct file {
    	union {
    		struct llist_node	fu_llist;
    		struct rcu_head 	fu_rcuhead;
    	} f_u;
    	struct path		f_path;
    
    //******所有的这些file文件结构全部都必须只能指向一个inode结构体
    	struct inode		*f_inode;	/* cached value */
    
    //******对应cdev结构体中的ops
    	const struct file_operations	*f_op;
    
    	/*
    	 * Protects f_ep_links, f_flags.
    	 * Must not be taken from IRQ context.
    	 */
    	spinlock_t		f_lock;
    	enum rw_hint		f_write_hint;
    	atomic_long_t		f_count;
    	unsigned int 		f_flags;
    	fmode_t			f_mode;
    	struct mutex		f_pos_lock;
    
    //******当前读写文件的位置
    	loff_t			f_pos;
    	struct fown_struct	f_owner;
    	const struct cred	*f_cred;
    	struct file_ra_state	f_ra;
    
    	u64			f_version;
    #ifdef CONFIG_SECURITY
    	void			*f_security;
    #endif
    	/* needed for tty driver, and maybe others */
    
    	//******常用来保存自定义设备结构体的地址
    void			*private_data;
    
    #ifdef CONFIG_EPOLL
    	/* Used by fs/eventpoll.c to link all the hooks to this file */
    	struct list_head	f_ep_links;
    	struct list_head	f_tfile_llink;
    #endif /* #ifdef CONFIG_EPOLL */
    	struct address_space	*f_mapping;
    	errseq_t		f_wb_err;
    } __randomize_layout
     __attribute__((aligned(4)));	/* lest something weird decides that 2 is OK */

## (2)file operation定义字符设备驱动提供给VFS的接口函数

[](http://blog.csdn.net/zqixiao_09/article/details/50850475)

    struct file_operations { 
    
     struct module *owner;//拥有该结构的模块的指针，一般为THIS_MODULES 
    
    
     loff_t (*llseek) (struct file *, loff_t, int);//用来修改文件当前的读写位置 
    
     ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);//从设备中同步读取数据 
    
     ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);//向设备发送数据
    
     ssize_t (*aio_read) (struct kiocb *, const struct iovec *, unsigned long, loff_t);//初始化一个异步的读取操作 
    
     ssize_t (*aio_write) (struct kiocb *, const struct iovec *, unsigned long, loff_t);//初始化一个异步的写入操作 
    
     int (*readdir) (struct file *, void *, filldir_t);//仅用于读取目录，对于设备文件，该字段为NULL 
    
     unsigned int (*poll) (struct file *, struct poll_table_struct *); //轮询函数，判断目前是否可以进行非阻塞的读写或写入 
    
     int (*ioctl) (struct inode *, struct file *, unsigned int, unsigned long); //执行设备I/O控制命令 
    
     long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long); //不使用BLK文件系统，将使用此种函数指针代替ioctl 
    
     long (*compat_ioctl) (struct file *, unsigned int, unsigned long); //在64位系统上，32位的ioctl调用将使用此函数指针代替 
    
    
     int (*mmap) (struct file *, struct vm_area_struct *); //用于请求将设备内存映射到进程地址空间
     
     int (*open) (struct inode *, struct file *); //打开 
    
     int (*flush) (struct file *, fl_owner_t id); 
    
     int (*release) (struct inode *, struct file *); //关闭 
    
     int (*fsync) (struct file *, struct dentry *, int datasync); //刷新待处理的数据 
    
     int (*aio_fsync) (struct kiocb *, int datasync); //异步刷新待处理的数据 
    
     int (*fasync) (int, struct file *, int); //通知设备FASYNC标志发生变化 
    
     int (*lock) (struct file *, int, struct file_lock *); 
    
     ssize_t (*sendpage) (struct file *, struct page *, int, size_t, loff_t *, int); 
    
     unsigned long (*get_unmapped_area)(struct file *, unsigned long, unsigned long, unsigned long, unsigned long); 
    
     int (*check_flags)(int); 
    
     int (*flock) (struct file *, int, struct file_lock *);
     
     ssize_t (*splice_write)(struct pipe_inode_info *, struct file *, loff_t *, size_t, unsigned int);
     
     ssize_t (*splice_read)(struct file *, loff_t *, struct pipe_inode_info *, size_t, unsigned int); 
    
     int (*setlease)(struct file *, long, struct file_lock **); 
    
    };

## (3)struct inode

[](http://blog.csdn.net/kokodudu/article/details/17201823)

    struct inode {
    	umode_t			i_mode;
    	unsigned short		i_opflags;
    	kuid_t			i_uid;
    	kgid_t			i_gid;
    	unsigned int		i_flags;
    
    #ifdef CONFIG_FS_POSIX_ACL
    	struct posix_acl	*i_acl;
    	struct posix_acl	*i_default_acl;
    #endif
    
    	const struct inode_operations	*i_op;
    	struct super_block	*i_sb;
    	struct address_space	*i_mapping;
    
    #ifdef CONFIG_SECURITY
    	void			*i_security;
    #endif
    
    	/* Stat data, not accessed from path walking */
    	unsigned long		i_ino;
    	/*
    	 * Filesystems may only read i_nlink directly. They shall use the
    	 * following functions for modification:
    	 *
    	 * (set|clear|inc|drop)_nlink
    	 * inode_(inc|dec)_link_count
    	 */
    	union {
    		const unsigned int i_nlink;
    		unsigned int __i_nlink;
    	};
    
    //****** i_rdev如果是设备文件,有一个设备号
    	dev_t			i_rdev;
    	loff_t			i_size;
    	struct timespec		i_atime;
    	struct timespec		i_mtime;
    	struct timespec		i_ctime;
    	spinlock_t		i_lock;	/* i_blocks, i_bytes, maybe i_size */
    	unsigned short i_bytes;
    	unsigned int		i_blkbits;
    	enum rw_hint		i_write_hint;
    	blkcnt_t		i_blocks;
    
    #ifdef __NEED_I_SIZE_ORDERED
    	seqcount_t		i_size_seqcount;
    #endif
    
    	/* Misc */
    	unsigned long		i_state;
    	struct rw_semaphore	i_rwsem;
    
    	unsigned long		dirtied_when;	/* jiffies of first dirtying */
    	unsigned long		dirtied_time_when;
    
    	struct hlist_node	i_hash;
    	struct list_head	i_io_list;	/* backing dev IO list */
    #ifdef CONFIG_CGROUP_WRITEBACK
    	struct bdi_writeback	*i_wb;		/* the associated cgroup wb */
    
    	/* foreign inode detection, see wbc_detach_inode() */
    	int			i_wb_frn_winner;
    	u16			i_wb_frn_avg_time;
    	u16			i_wb_frn_history;
    #endif
    	struct list_head	i_lru;		/* inode LRU list */
    	struct list_head	i_sb_list;
    	struct list_head	i_wb_list;	/* backing dev writeback list */
    	union {
    		struct hlist_head	i_dentry;
    		struct rcu_head		i_rcu;
    	};
    	u64			i_version;
    	atomic_t		i_count;
    	atomic_t		i_dio_count;
    	atomic_t		i_writecount;
    #ifdef CONFIG_IMA
    	atomic_t		i_readcount; /* struct files open RO */
    #endif
    	const struct file_operations	*i_fop;	/* former ->i_op->default_file_ops */
    	struct file_lock_context	*i_flctx;
    	struct address_space	i_data;
    	struct list_head	i_devices;
    	union {
    		struct pipe_inode_info	*i_pipe;
    		struct block_device	*i_bdev;
    
    //******指向该设备对应的cdev结构
    		struct cdev		*i_cdev;
    		char			*i_link;
    		unsigned		i_dir_seq;
    	};

## (4)cdev结构体描述字符设备 c = char

    <include/linux/cdev.h> 
     
    struct cdev { 
     struct kobject kobj; //内嵌的内核对象. 
     struct module *owner; //该字符设备所在的内核模块的对象指针. 
     const struct file_operations *ops; //该结构描述了字符设备所能实现的方法，是极为关键的一个结构体. 
     struct list_head list; //用来将已经向内核注册的所有字符设备形成链表. 
     dev_t dev; //字符设备的设备号，由主设备号和次设备号构成. 
     unsigned int count; //隶属于同一主设备号的次设备号的个数. 
    };

    //初始化cdev结构体方法1
    void cdev_init(struct cdev *cdev, const struct file_operations *fops) 
    { 
     memset(cdev, 0, sizeof *cdev); 
     INIT_LIST_HEAD(&cdev->list); 
     kobject_init(&cdev->kobj, &ktype_cdev_default); 
     cdev->ops = fops; 
    }
    
     [//INIT_LIST_HEAD宏](//init_LIST_HEAD宏) 
    INIT_LIST_HEAD(struct list_head *list)
    {
     list->next = list->prev = list;
    }

    //初始化cdev结构体方法2 适用于cdev指针
    struct cdev *cdev_alloc(void) 
    { 
     struct cdev *p = kzalloc(sizeof(struct cdev), GFP_KERNEL); 
     if (p) { 
     INIT_LIST_HEAD(&p->list); 
     kobject_init(&p->kobj, &ktype_cdev_dynamic); 
     } 
     return p; 
    }

    //向系统添加cdev
    int cdev_add(struct cdev *p, dev_t dev, unsigned count)
    {
    	int error;
    
    	p->dev = dev;
    	p->count = count;
    
    //在cdev_map这张哈希表中建立dev的映射,实现设备号到cdev的查找
    	error = kobj_map(cdev_map, dev, count, NULL,
    			 exact_match, exact_lock, p);
    	if (error)
    		return error;
    
    	kobject_get(p->kobj.parent);
    
    	return 0;
    }

    //撤销cdev结构
    void cdev_del(struct cdev *p)
    {
    //解除映射
    	cdev_unmap(p->dev, p->count);
    ////将kobj 对象的引用计数减1，如果引用计数降为0，则调用kobject_release()释放该kobject 对象;
    	kobject_put(&p->kobj);
    }

## (5)设备号相关操作

一个字符设备或块设备都有一个主设备号和一个次设备号。

主设备号用来标识与设备文件相连的驱动程序，用来反映设备类型。次设备号被驱动程序用来辨别操作的是哪个设备，用来区分同类型的设备。

    //静态申请
    //dev_t from 要分配的起始设别号(已知)
    int register_chrdev_region(dev_t from, unsigned count, const char *name) 
    { 
     struct char_device_struct *cd; 
     dev_t to = from + count; 
     dev_t n, next; 
     
     for (n = from; n < to; n = next) { 
     next = MKDEV(MAJOR(n)+1, 0); 
     if (next > to) 
     next = to; 
     cd = __register_chrdev_region(MAJOR(n), MINOR(n), 
     next - n, name); 
     if (IS_ERR(cd)) 
     goto fail; 
     } 
     return 0; 
    fail: 
     to = n; 
     for (n = from; n < to; n = next) { 
     next = MKDEV(MAJOR(n)+1, 0); 
     kfree(__unregister_chrdev_region(MAJOR(n), MINOR(n), next - n)); 
     } 
     return PTR_ERR(cd); 
    }

    //动态申请
    int alloc_chrdev_region(dev_t *dev, unsigned baseminor, unsigned count, 
     const char *name) 
    { 
     struct char_device_struct *cd; 
     cd = __register_chrdev_region(0, baseminor, count, name); 
     if (IS_ERR(cd)) 
     return PTR_ERR(cd); 
     *dev = MKDEV(cd->major, cd->baseminor); 
     return 0; 
    }

    //注销设备号
    void unregister_chrdev_region(dev_t from, unsigned count)
    {
    	dev_t to = from + count;
    	dev_t n, next;
    
    	for (n = from; n < to; n = next) {
    		next = MKDEV(MAJOR(n)+1, 0);
    		if (next > to)
    			next = to;
    		kfree(__unregister_chrdev_region(MAJOR(n), MINOR(n), next - n));
    	}
    }

    __register_chrdev_region(unsigned int major, unsigned int baseminor,
    			 int minorct, const char *name)
    {
    	struct char_device_struct *cd, **cp;
    	int ret = 0;
    	int i;
    
    	cd = kzalloc(sizeof(struct char_device_struct), GFP_KERNEL);
    	if (cd == NULL)
    		return ERR_PTR(-ENOMEM);
    
    	mutex_lock(&chrdevs_lock);
    
    	/* temporary */
    	if (major == 0) {
    		for (i = ARRAY_SIZE(chrdevs)-1; i > 0; i--) {
    			if (chrdevs[i] == NULL)
    				break;
    		}
    
    		if (i < CHRDEV_MAJOR_DYN_END)
    			pr_warn("CHRDEV \"%s\" major number %d goes below the dynamic allocation range\n",
    				name, i);
    
    		if (i == 0) {
    			ret = -EBUSY;
    			goto out;
    		}
    		major = i;
    	}
    
    	cd->major = major;
    	cd->baseminor = baseminor;
    	cd->minorct = minorct;
    	strlcpy(cd->name, name, sizeof(cd->name));
    
    	i = major_to_index(major);
    
    	for (cp = &chrdevs[i]; *cp; cp = &(*cp)->next)
    		if ((*cp)->major > major ||
    		 ((*cp)->major == major &&
    		 (((*cp)->baseminor >= baseminor) ||
    		 ((*cp)->baseminor + (*cp)->minorct > baseminor))))
    			break;
    
    	/* Check for overlapping minor ranges. */
    	if (*cp && (*cp)->major == major) {
    		int old_min = (*cp)->baseminor;
    		int old_max = (*cp)->baseminor + (*cp)->minorct - 1;
    		int new_min = baseminor;
    		int new_max = baseminor + minorct - 1;
    
    		/* New driver overlaps from the left. */
    		if (new_max >= old_min && new_max <= old_max) {
    			ret = -EBUSY;
    			goto out;
    		}
    
    		/* New driver overlaps from the right. */
    		if (new_min <= old_max && new_min >= old_min) {
    			ret = -EBUSY;
    			goto out;
    		}
    	}
    
    	cd->next = *cp;
    	*cp = cd;
    	mutex_unlock(&chrdevs_lock);
    	return cd;
    out:
    	mutex_unlock(&chrdevs_lock);
    	kfree(cd);
    	return ERR_PTR(ret);
    }

## (6) copy_from_user和copy_to_user

     [//cop](//copt) y_from_user
    static inline int copy_from_user(void *to, const void __user volatile *from, 
     unsigned long n) 
    { 
    		//首先检查用户空间的地址指针是否有效
     __chk_user_ptr(from, n); 
     volatile_memcpy(to, from, n); 
     return 0; 
    } 
     
    
    //copy_to_user
    static inline int copy_to_user(void __user volatile *to, const void *from, 
     unsigned long n) 
    { 
    		//首先检查用户空间的地址指针是否有效
     __chk_user_ptr(to, n); 
     volatile_memcpy(to, from, n); 
     return 0; 
    }
    
    //_memcpy() 函数
    static void volatile_memcpy(volatile char *to, const volatile char *from, 
     unsigned long n) 
    { 
     while (n--) 
     *(to++) = *(from++); 
    }

