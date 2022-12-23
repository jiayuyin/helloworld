#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/jiffies.h>


#define TIMER_COUNT 1
#define TIMER_NAME "timer"

#define CLOSE_CMD   _IO(0XEF, 1)    /*关闭命令*/
#define OPEN_CMD    _IO(0XEF, 2)    /*打开命令*/
#define SETPERIOD_CMD   _IOW(0XEF, 3, int) /*设置周期命令*/


/*设备结构体*/
struct timer_dev{
    dev_t devid;    /*设备号*/
    int major;  /*主设备号*/
    int minor;  /*次设备号*/
    struct cdev cdev;   /*字符设备*/
    struct class *class;    /*创建类*/
    struct device *device;  /*创建设备*/
    struct device_node *node; /*设备节点*/
    int led_gpio;   /*led号*/
    int timerperiod; /*周期时间*/
    struct timer_list timer; /*定时器*/
    spinlock_t lock; /* 定义自旋锁 */
};


struct timer_dev timer;


static int timer_open(struct inode *inode, struct file *filp){
    filp->private_data = &timer; /* 设置私有数据 */
    return 0;
}
 
static long timer_ioctl(struct file *filp,unsigned int cmd, unsigned long arg){
    int ret = 0;
    int value = 0;
    struct timer_dev *dev = (struct timer_dev*)filp->private_data;


    switch(cmd) {
    case CLOSE_CMD:
        del_timer_sync(&dev->timer);
        break;
    case OPEN_CMD:
        mod_timer(&dev->timer, jiffies + msecs_to_jiffies(timer.timerperiod));
        break;
    case SETPERIOD_CMD:
        ret = copy_from_user(&value, (int *)arg, sizeof(int));
        if(ret < 0){
            return -EFAULT;
        }
        dev->timerperiod = value;
        mod_timer(&dev->timer, jiffies + msecs_to_jiffies(timer.timerperiod));
        break;
    }
    return ret;
}
 
static int timer_release(struct inode *inode, struct file *filp){
    
    return 0;
}



static const struct file_operations timer_fops = { /*字符设备操作函数集合*/
    .owner = THIS_MODULE,
    .unlocked_ioctl = timer_ioctl,
    .open = timer_open,
    .release = timer_release,
    
};

/*定时器超市处理函数*/
static void timer_func(unsigned long arg) {
    struct timer_dev *dev = (struct timer_dev*)arg;
    static int sta = 1;
    unsigned long flags;

    sta = !sta;
    spin_lock_irqsave(&dev->lock, flags);
    gpio_set_value(dev->led_gpio, sta);
    spin_unlock_irqrestore(&dev->lock, flags);
    mod_timer(&dev->timer, jiffies + msecs_to_jiffies(timer.timerperiod));

}

/*初始化LED灯*/
int led_init(struct timer_dev *dev){
        int ret = 0;

       /*获取设备节点*/
        dev->node = of_find_node_by_path("/gpioled");
        if(dev->node == NULL){    /*寻找节点失败*/
            ret = -EINVAL;
            goto failed_findnode;
        }

        /*获取led所对应的gpio*/
        dev->led_gpio = of_get_named_gpio(dev->node, "led-gpios", 0);
        if(dev->led_gpio < 0){
            printk("can't find led gpio \r\n");
            ret = -EINVAL;
            goto failed_findnode;
        }

        printk("led gpio num = %d \r\n",dev->led_gpio);

        /*申请gpio*/
        ret = gpio_request(dev->led_gpio, "led-gpios");
        if(ret){
            printk("Failed to request gpio \r\n");
            ret = -EINVAL;
            goto failed_findnode;
        }

        /*使用IO，申请为输出*/
        ret = gpio_direction_output(dev->led_gpio, 1); /*设置为输出，高电平不点亮*/
        if(ret < 0){
            goto failed_setoutput;
        }



        return 0;

failed_setoutput:
        gpio_free(dev->led_gpio);
failed_findnode:
        device_destroy(dev->class, dev->devid);
        return ret;
}

/*入口函数*/
static int __init timer_init(void){
    int ret = 0;

    /* 初始化自旋锁 */
spin_lock_init(&timer.lock);

    /*注册字符设备*/
    timer.major = 0;     /*内核自动申请设备号*/
    if(timer.major){     /*如果定义了设备号*/
        timer.devid = MKDEV(timer.major, 0);
        ret = register_chrdev_region(timer.devid, TIMER_COUNT, TIMER_NAME); 
    }
    else{   /*否则自动申请设备号*/
        ret = alloc_chrdev_region(&timer.devid, 0, TIMER_COUNT, TIMER_NAME);
        timer.major = MAJOR(timer.devid); /*保存主设备号*/
        timer.minor = MINOR(timer.devid); /*保存次设备号*/
    }
    if(ret < 0){
        goto failed_devid;
    }
    printk("timerdev major = %d minor = %d \r\n",timer.major,timer.minor);  /*打印主次设备号*/

    /*添加字符设备*/
    timer.cdev.owner = THIS_MODULE;
    cdev_init(&timer.cdev, &timer_fops);
    ret = cdev_add(&timer.cdev, timer.devid, TIMER_COUNT);
    if(ret < 0){    /*添加字符设备失败*/
        goto failed_cdev;
    }

    /*自动添加设备节点*/
    /*创建类*/
    timer.class = class_create(THIS_MODULE, TIMER_NAME); /*class_creat(owner,name);*/
    if(IS_ERR(timer.class)){   /*判断是否创建类成功*/
        ret = PTR_ERR(timer.class);
        goto failed_class;
    }

    /*创建设备*/

    timer.device = device_create(timer.class, NULL, timer.devid, NULL, TIMER_NAME);
     if(IS_ERR(timer.device)){   /*判断是否创建类成功*/
        ret = PTR_ERR(timer.device);
        goto failed_device;
    }

    /*初始化led*/
    ret = led_init(&timer);
    if(ret < 0){
        goto failed_ledinit;
    }

    /*初始化定时器*/
    init_timer(&timer.timer);
    timer.timerperiod = 500;
    timer.timer.function = timer_func;  /*定时器超时处理函数*/
    timer.timer.expires = jiffies + msecs_to_jiffies(timer.timerperiod);/*超时时间*/
    timer.timer.data = (unsigned long)&timer;   /*将指针变量强制转化为unsigned long类型*/

    add_timer(&timer.timer);    /*添加定时器*/

    return 0;


failed_ledinit:
failed_device:
    class_destroy(timer.class);
failed_class:
    cdev_del(&timer.cdev);
failed_cdev:
    unregister_chrdev_region(timer.devid, TIMER_COUNT);
failed_devid:
    return ret;
}

/*出口函数*/
static void __exit timer_exit(void){

    /*删除定时器*/
    del_timer(&timer.timer);

    gpio_free(timer.led_gpio);
    /*注销字符设备*/
    cdev_del(&timer.cdev);
    /*卸载设备*/
    unregister_chrdev_region(timer.devid, TIMER_COUNT);

    device_destroy(timer.class, timer.devid);
    class_destroy(timer.class);

}


/*模块入口和出口*/
module_init(timer_init);
module_exit(timer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ZYC");