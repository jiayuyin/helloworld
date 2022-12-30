socket  

*在开发busybox中syslogd命令时用到了socket*  


首先，了解到了两个常用的结构体

sockaddr 和 sockaddr_in  

```c
struct sockaddr {
    unsigned short sa_family; /* address family, AF_xxx */
    char sa_data[14]; /* 14 bytes of protocol address */
};
```
sa_family ：是2字节的地址家族，一般都是“AF_xxx”的形式，它的值包括三种：AF_INET，AF_INET6和AF_UNSPEC。

```c
struct sockaddr_in {
    short int sin_family; /* Address family 在socket编程中只能是AF_INET */
    unsigned short int sin_port; /* Port number（使用网络字节顺序） */
    struct in_addr sin_addr; /* Internet address */
    unsigned char sin_zero[8]; /* Same size as struct sockaddr */
};
```

其中in_addr：  
```c
typedef struct in_addr {
    union {
        struct{ unsigned char s_b1,s_b2, s_b3,s_b4;} s_un_b;
        struct{ unsigned short s_w1, s_w2;} s_un_w;
        unsigned long s_addr; // 一般都是这个
    } S_un;
} IN_ADDR;
```

给in_addr赋值的一种最简单方法是使用inet_addr函数，它可以把一个代表IP地址的字符串赋值转换为in_addr类型，如
`addrto.sin_addr.s_addr=inet_addr("192.168.0.2");`

htons()作用是将端口号由主机字节序转换为网络字节序的整数值。(host to net)  
`sin.sin_port = htos(514);`  
inet_addr()作用是将一个IP字符串转化为一个网络字节序的整数值，用于sockaddr_in.sin_addr.s_addr。  
`sin.sin_addr.s_addr = inet_addr("192.168.1.1");`  
inet_ntoa()作用是将一个sin_addr结构体输出成IP字符串(network to ascii)。  
`ip_str = inet_ntoa(sin.sin_addr.s_addr);`

---
首先得包含两个头文件
```c
#include <sys/types.h>
#include <socket.h>
```

## socket函数：  

`int socket(int domain ,int type, int protocol);`


1. domain:  
AF_INET 这是大多数用来产生socket的协议，使用TCP或UDP来传输，用IPv4的地址  
AF_INET6 与上面类似，不过是来用IPv6的地址  
AF_UNIX 本地协议，使用在Unix和Linux系统上，一般都是当客户端和服务器在同一台及其上的时候使用  

2. int type：  
SOCK_STREAM 这个协议是按照顺序的、可靠的、数据完整的基于字节流的连接。这是一个使用最多的socket类型，这个socket是使用TCP来进行传输。  
SOCK_DGRAM 这个协议是无连接的、固定长度的传输调用。该协议是不可靠的，使用UDP来进行它的连接。  
SOCK_SEQPACKET该协议是双线路的、可靠的连接，发送固定长度的数据包进行传输。必须把这个包完整的接受才能进行读取。  
SOCK_RAW socket类型提供单一的网络访问，这个socket类型使用ICMP公共协议。（ping、traceroute使用该协议）  
SOCK_RDM 这个类型是很少使用的，在大部分的操作系统上没有实现，它是提供给数据链路层使用，不保证数据包的顺序  

3. protocol：  
传0 表示使用默认协议。

4. 返回值：  
成功：返回指向新创建的socket的文件描述符，失败：返回-1，设置errno  

文件描述符的概念：  
内核利用文件描述符来访问文件。文件描述符是非负整数。打开现存文件或新建文件时，内核会返回一个文件描述符。读写文件也需要使用文件描述符来指定待读写的文件。

socket()打开一个网络通讯端口，如果成功的话，就像open()一样返回一个文件描述符，应用程序可以像读写文件一样用read/write在网络上收发数据，如果socket()调用出错则返回-1。  
对于IPv4，domain参数指定为AF_INET。  
对于TCP协议，type参数指定为SOCK_STREAM，表示面向流的传输协议。  
如果是UDP协议，则type参数指定为SOCK_DGRAM，表示面向数据报的传输协议。  
protocol参数的介绍从略，指定为0即可。

---

## bind函数：  

`int bind(int sockfd, const struct sockaddr * addr, socklen_t addrlen);`  

1. sockfd：  
sockfd 文件描述符  

2. sockaddr 结构体

3. addrlen：  
sizeof（addr）长度

4. 返回值：  
成功返回0，失败返回-1，设置errno  

服务器程序所监听的网络地址和端口号通常是固定不变的，客户端程序得知服务器程序的地址和端口号后就可以向服务器发起连接，因此服务器需要调用bind绑定一个固定的网络地址和端口号。  

bind()的作用是将参数sockfd和addr绑定在一起，使sockfd这个用于网络通讯的文件描述符监听addr所描述的地址和端口号。前面讲过，struct sockaddr *是一个通用指针类型，addr参数实际上可以接受多种协议的sockaddr结构体，而它们的长度各不相同，所以需要第三个参数addrlen指定结构体的长度。如：  
```c
struct sockaddr_in server_addr;
sock_fd = sock(AF_INET, SOCK_STREAM, 0);
bzero(&server_addr, sizeof(server_addr)); //使用前一定记得清零，不然可能会有意想不到的错误
server_addr.sin_family = AF_INET;
server_addr.sin_port = htols(SERVER_PORT);
server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
```

## setsockopt
`int setsockopt(int socket, int level, int option_name,const void *option_value, size_t ，ption_len);`  
