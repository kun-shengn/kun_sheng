# 智能安全帽
本示例将演示如何在BearPi-HM_Nano开发板上使用socket编程创建tcp客户端，回复当前采集数据。

## socket API分析
本案例主要使用了以下几个API完socket编程实验
## socket()

```c
sock_fd = socket(AF_INET, SOCK_STREAM, 0)) //AF_INT:ipv4, SOCK_STREAM:tcp协议
```
**描述：**

在网络编程中所需要进行的第一件事情就是创建一个socket，无论是客户端还是服务器端，都需要创建一个socket，该函数返回socket文件描述符，类似于文件描述符。socket是一个结构体，被创建在内核中。
## sendto()
```c
int sendto ( socket s , const void * msg, int len, unsigned int flags,
const struct sockaddr * to , int tolen ) ;
```
**描述：**

sendto() 用来将数据由指定的socket传给对方主机。参数s为已建好连线的socket。参数msg指向欲连线的数据内容，参数flags 一般设0，

## recvfrom()
```c
int recvfrom(int s, void *buf, int len, unsigned int flags, struct sockaddr *from, int *fromlen);
```
**描述：**
从指定地址接收UDP数据报。


**参数：**

|名字|描述|
|:--|:------| 
| s | socket描述符.  |
| buf | UDP数据报缓存地址.  |
| len | UDP数据报长度.  |
| flags | 该参数一般为0.  |
| from | 对方地址  |
| fromlen | 对方地址长度.  |



## 软件设计

**主要代码分析**

完成Wifi热点的连接需要以下几步

1. 通过 `socket` 接口创建一个socket,`AF_INT`表示ipv4,`SOCK_STREAM`表示使用tcp协议
2. 调用 `sendto` 接口发送数据到服务端。
3. 调用`CheckHandler方法`与`SensorHandler方法`进行数据的采集与佩戴检测

```c
static void TCPClientTask(void)
{
    
    GpioInit();
    uint32_t ret;

    WifiIotUartAttribute uart_attr = {

        //baud_rate: 9600
        .baudRate = 9600,

        //data_bits: 8bits
        .dataBits = 8,
        .stopBits = 1,
        .parity = 0,
    };

    //Initialize uart driver
    ret = UartInit(WIFI_IOT_UART_IDX_1, &uart_attr, NULL);
    if (ret != WIFI_IOT_SUCCESS)
    {
        printf("Failed to init uart! Err code = %d\n", ret);
        return;
    }
    //初始化LED灯
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_11, WIFI_IOT_IO_FUNC_GPIO_11_GPIO);
    GpioSetDir(WIFI_IOT_IO_NAME_GPIO_11, WIFI_IOT_GPIO_DIR_IN);
    IoSetPull(WIFI_IOT_IO_NAME_GPIO_11, WIFI_IOT_IO_PULL_DOWN);
    CheckHandler();
    SensorHandler();
    //服务器的地址信息
    struct sockaddr_in send_addr;
    socklen_t addr_length = sizeof(send_addr);
    char recvBuf[512];

    //连接Wifi
    WifiConnect("BearPi", "0987654321");

    //创建socket
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("create socket failed!\r\n");
        exit(1);
    }

    //初始化预连接的服务端地址
    send_addr.sin_family = AF_INET;
    send_addr.sin_port = htons(_PROT_);
    send_addr.sin_addr.s_addr = inet_addr("192.168.1.1");
    addr_length = sizeof(send_addr);
    connect(sock_fd,(struct sockaddr *)&send_addr, addr_length);
    //总计发送 count 次数据
    while (1)
    {
        bzero(recvBuf, sizeof(recvBuf));

        //发送数据到服务远端
        send(sock_fd, send_data, strlen(send_data), 0);

        //线程休眠一段时间
       sleep(1);

        //接收服务端返回的字符串
        //recv(sock_fd, recvBuf, sizeof(recvBuf), 0);
        //printf("%s:%d=>%s\n", inet_ntoa(send_addr.sin_addr), ntohs(send_addr.sin_port), recvBuf);
    
    }
        closesocket(sock_fd);

    //关闭这个 socket
}
```

