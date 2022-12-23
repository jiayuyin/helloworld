## Linux内核定时器

*在开发设备启动闪灯的时候用到了定时器。*

Linux 内核中有大量的函数需要时间管理，比如周期性的调度程序、延时程序、对于驱动编写者来说最常用的就是定时器。硬件定时器提供时钟源，时钟源的频率可以设置， 设置好以后就周期性的产生定时中断，系统使用定时中断来计时。中断周期性产生的频率就是系统频率，也叫做节拍率(tick rate)(有的资料也叫系统频率)。

 Linux 内核使用全局变量 jiffies 来记录系统从启动以来的系统节拍数，系统启动的时候会
将 jiffies 初始化为 0， jiffies 定义在文件 include/linux/jiffies.h 中，定义如下：

```
extern u64 __jiffy_data jiffies_64;
extern unsingned long volatile __jiffy_data jiffies;
```

### 一、内核定时器

Linux 内核定时器采用系统时钟来实现，与硬件定时器功能一样，当超时时间到了以后设
置的定时处理函数就会执行。内核定时器不需要一大堆寄存器的配置工作，并且内核定时器执行完超时处理函数以后就会自动关闭。若需要周期运行，则需要在处理函数中再次打开内核定时器。

    内核定时器和硬件定时器共同点：
            ① 超时时间到了以后，会执行处理函数
            不同点:
            ① 内核定时器不需要配置寄存器，硬件定时器需要配置对应寄存器。
            ② 内核定时器只执行一次处理函数，若需要周期执行，需要在处理函数中再次打开。硬件定时

Linux 内核使用 timer_list 结构体表示内核定时器， timer_list 定义在文件include/linux/timer.h 中，定义如下：
```
struct timer_list {
    struct list_head entry;
    unsigned long expires; /* 定时器超时时间，单位是节拍数 */
    struct tvec_base *base;
    void (*function)(unsigned long); /* 定时处理函数 */
    unsigned long data; /* 要传递给 function 函数的参数 */
    int slack;
};
```


### 二、内核定时器API函数  
1、init_timer 函数

    void init_timer(struct timer_list *timer)
    函数参数和返回值含义如下：
    timer：要初始化定时器。
    返回值： 没有返回值

init_timer 函数负责初始化 timer_list 类型变量，当定义完timer_list结构体以后，需要使用此函数进行初始化。

2、add_timer 函数

    void add_timer(struct timer_list *timer)
    函数参数和返回值含义如下：
    timer：要注册的定时器。
    返回值： 没有返回值。

add_timer 函数用于向 Linux 内核注册定时器，使用 add_timer 函数向内核注册定时器以后，
定时器就会开始运行。 

3、del_timer 函数

    int del_timer(struct timer_list * timer)
    函数参数和返回值含义如下：
    timer：要删除的定时器。
    返回值： 0，定时器还没被激活； 1，定时器已经激活。

del_timer 函数用于删除一个定时器，不管定时器有没有被激活，都可以使用此函数删除。

4、del_timer_sync 函数

    int del_timer_sync(struct timer_list *timer)
    函数参数和返回值含义如下：
    timer：要删除的定时器。
    返回值： 0，定时器还没被激活； 1，定时器已经激活。

del_timer_sync 函数是 del_timer 函数的同步版，会等待其他处理器使用完定时器再删除，

5、mod_timer 函数

    int mod_timer(struct timer_list *timer, unsigned long expires)
    函数参数和返回值含义如下：
    timer：要修改超时时间(定时值)的定时器。
    expires：修改后的超时时间。
    返回值： 0，调用 mod_timer 函数前定时器未被激活； 1，调用 mod_timer 函数前定时器已
    被激活

mod_timer 函数用于修改定时值，如果定时器还没有激活的话， mod_timer 函数会激活定时
器,通常使用此函数达到定时器周期循环的目的。


三、内核定时器使用框架

        如下图所示是一个周期性定式循环框架。
```c
struct timer_list timer; /* 定义定时器 */
/* 定时器回调函数 */
void function(unsigned long arg)
{
    /*
     * 定时器处理代码
     */

    /* 如果需要定时器周期性运行的话就使用 mod_timer
     * 函数重新设置超时值并且启动定时器。
     */
    mod_timer(&dev->timertest, jiffies + msecs_to_jiffies(2000));
}

/* 初始化函数 */
void init(void)
{
    init_timer(&timer);                              /* 初始化定时器 */
    timer.function = function;                       /* 设置定时处理函数 */
    timer.expires = jffies + msecs_to_jiffies(2000); /* 超时时间 2 秒 */
    timer.data = (unsigned long)&dev;                /* 将设备结构体作为参数 */

    add_timer(&timer); /* 启动定时器 */
}

/* 退出函数 */
void exit(void)
{
    del_timer(&timer); /* 删除定时器 */
    /* 或者使用 */
    del_timer_sync(&timer);
}
```