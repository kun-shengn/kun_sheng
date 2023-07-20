# 信标
本示例将演示如何在BearPi-HM_Nano开发板上使用socket编程创建TCP服务端，接收客户端消息并显示对应数据。

## socket API分析
本案例主要使用了以下几个API完socket编程实验
## socket()

```c
sock_fd = socket(AF_INET, SOCK_STREAM, 0)) //AF_INT:ipv4, SOCK_STREAM:tcp协议
```
**描述：**

在网络编程中所需要进行的第一件事情就是创建一个socket，无论是客户端还是服务器端，都需要创建一个socket，该函数返回socket文件描述符，类似于文件描述符。socket是一个结构体，被创建在内核中。
## bind()
```c
bind(sockfd,(struct sockaddr*)&serveraddr,sizeof(serveraddr))
```
**描述：**

把一个本地协议地址和套接口绑定，比如把本机的2222端口绑定到套接口。注意：为什么在上图中客户端不需要调用bind函数？这是因为如果没有调用bind函数绑定一个端口的话，当调用connect函数时，内核会为该套接口临时选定一个端口，因此可以不用绑定。而服务器之所以需要绑定的原因就是，所以客户端都需要知道服务器使用的哪个端口，所以需要提前绑定


## listen()
```c
int listen(int s, int backlog)
```
**描述：**

当socket创建后，它通常被默认为是主动套接口，也就是说是默认为要马上调用connect函数的，而作为服务器是需要被动接受的，所以需要调用linsten函数将主动套接口转换成被动套接口。调用linsten函数后，内核将从该套接口接收连接请求。



## accept()
```c
int accept(s, addr, addrlen)    
```
**描述：**

此函数返回已经握手完成的连接的套接口。注意：此处的套接口不同于服务器开始创建的监听套接口，此套接口是已经完成连接的套接口，监听套接口只是用来监听。

## recv()
```c
int recv( SOCKET s, char *buf, int  len, int flags)   
```
**描述：**

recv函数用来从TCP连接的另一端接收数据

## send()
```c
int send( SOCKET s,char *buf,int len,int flags )
```
**描述：**
send函数用来向TCP连接的另一端发送数据。




## 软件设计

**主要代码分析**

完成Wifi热点的连接需要以下几步

1. 通过 `socket` 接口创建一个socket,`AF_INT`表示ipv4,`SOCK_STREAM`表示使用tcp协议
2. 调用 `bind` 接口绑定socket和地址。
3. 调用 `listen` 接口监听(指定port监听),通知操作系统区接受来自客户端链接请求,第二个参数：指定队列长度
4. 调用`accept`接口从队列中获得一个客户端的请求链接
5. 调用 `recv` 接口接收客户端发来的数据
6. 调用`TcpClientHandler`方法为每一个新接入设备创建一个socket套接字进行信息传输。 

```c
static void TCPServerTask(void)
{

    WifiIotUartAttribute uart_attr = {

        //baud_rate: 9600
        .baudRate = 9600,

        //data_bits: 8bits
        .dataBits = 8,
        .stopBits = 1,
        .parity = 0,
    };
    uart_ret = UartInit(WIFI_IOT_UART_IDX_1, &uart_attr, NULL);
    if (uart_ret != WIFI_IOT_SUCCESS)
    {
        printf("Failed to init uart! Err code = %d\n", uart_ret);
        return;
    }
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_13, WIFI_IOT_IO_FUNC_GPIO_13_I2C0_SDA);
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_14, WIFI_IOT_IO_FUNC_GPIO_14_I2C0_SCL);
    I2cInit(WIFI_IOT_I2C_IDX_0, OLED_I2C_BAUDRATE);
    WatchDogDisable();
    usleep(20*1000);
    ssd1306_Init();
    ssd1306_Fill(Black);
    SensorHandler();
    int ap_fd;
    int addr_length;
    struct sockaddr_in send_addr;
    addr_length = sizeof(send_addr);
    WifiConnect("BearPi", "0987654321");
    if ((ap_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("create socket failed!\r\n");
        exit(1);
    }

    //初始化预连接的服务端地址
    send_addr.sin_family = AF_INET;
    send_addr.sin_port = htons(_PROT_);
    send_addr.sin_addr.s_addr = inet_addr("192.168.1.1");
    addr_length = sizeof(send_addr);
    connect(ap_fd,(struct sockaddr *)&send_addr, addr_length);

	g_wifiEventHandler.OnHotspotStaJoin = OnHotspotStaJoinHandler;
    g_wifiEventHandler.OnHotspotStaLeave = OnHotspotStaLeaveHandler;
    g_wifiEventHandler.OnHotspotStateChanged = OnHotspotStateChangedHandler;
    error = RegisterWifiEvent(&g_wifiEventHandler);
    if (error != WIFI_SUCCESS)
    {
        printf("RegisterWifiEvent failed, error = %d.\r\n",error);
		exit(1);
    }
    printf("RegisterWifiEvent succeed!\r\n");
	HotspotConfig config = {0};

    strcpy(config.ssid, AP_SSID);
    strcpy(config.preSharedKey, AP_PSK);
    config.securityType = WIFI_SEC_TYPE_PSK;
    config.band = HOTSPOT_BAND_TYPE_2G;
    config.channelNum = 7;

    error = SetHotspotConfig(&config);
    if (error != WIFI_SUCCESS)
    {
        printf("SetHotspotConfig failed, error = %d.\r\n", error);
        exit(1);
    }
    printf("SetHotspotConfig succeed!\r\n");

    //启动wifi热点模式
    error = EnableHotspot(); 
    if (error != WIFI_SUCCESS)
    {
        printf("EnableHotspot failed, error = %d.\r\n", error);
        exit(1);
    }
    printf("EnableHotspot succeed!\r\n");

    //检查热点模式是否使能
    if (IsHotspotActive() == WIFI_HOTSPOT_NOT_ACTIVE)
    {
        printf("Wifi station is not actived.\r\n");
        exit(1);
    }
    printf("Wifi station is actived!\r\n");

    //启动dhcp
    g_lwip_netif = netifapi_netif_find("ap0");
    if (g_lwip_netif) 
    {
        ip4_addr_t bp_gw;
        ip4_addr_t bp_ipaddr;
        ip4_addr_t bp_netmask;

        IP4_ADDR(&bp_gw, 192, 168, 1, 1);           /* input your gateway for example: 192.168.1.1 */
        IP4_ADDR(&bp_ipaddr, 192, 168, 1, 1);       /* input your IP for example: 192.168.1.1 */
        IP4_ADDR(&bp_netmask, 255, 255, 255, 0);    /* input your netmask for example: 255.255.255.0 */

        err_t ret = netifapi_netif_set_addr(g_lwip_netif, &bp_ipaddr, &bp_netmask, &bp_gw);
        if(ret != ERR_OK)
        {
            printf("netifapi_netif_set_addr failed, error = %d.\r\n", ret);
            exit(1);
        }
        printf("netifapi_netif_set_addr succeed!\r\n");

        ret = netifapi_dhcps_start(g_lwip_netif, 0, 0);
        if(ret != ERR_OK)
        { 
            printf("netifapi_dhcp_start failed, error = %d.\r\n", ret);
            exit(1);
        }
        printf("netifapi_dhcps_start succeed!\r\n");

    }

	//服务端地址信息
	struct sockaddr_in server_sock;
    int sin_size;

	//创建socket
	if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket is error\r\n");
		exit(1);
	}

	bzero(&server_sock, sizeof(server_sock));
	server_sock.sin_family = AF_INET;
	server_sock.sin_addr.s_addr = htonl(INADDR_ANY);
	server_sock.sin_port = htons(_PROT_);

	//调用bind函数绑定socket和地址
	if (bind(sock_fd, (struct sockaddr *)&server_sock, sizeof(struct sockaddr)) == -1)
	{
		perror("bind is error\r\n");
		exit(1);
	}

	//调用listen函数监听(指定port监听)
	if (listen(sock_fd, TCP_BACKLOG) == -1)
	{
		perror("listen is error\r\n");
		exit(1);
	}

	printf("start accept\n");
	//调用accept函数从队列中
	while (1)
	{
		sin_size = sizeof(struct sockaddr_in);

		if ((new_fd[client_no] = accept(sock_fd, (struct sockaddr *)&client_sock[client_no], (socklen_t *)&sin_size)) == -1)
		{
			perror("accept");
			continue;
		}
		cli_addr[client_no] = malloc(sizeof(struct sockaddr));
		printf("accept addr\r\n");

		if (cli_addr != NULL)
		{
			memcpy(cli_addr[client_no], &client_sock[client_no], sizeof(struct sockaddr));
		}
        client_no++; 

        TcpClientHandler();//服务器端多对一处理函数
	}
}

```

## 编译调试

### 修改 BUILD.gn 文件

修改 `applications\BearPi\BearPi-HM_Nano\sample` 路径下 BUILD.gn 文件，指定 `tcp_server` 参与编译。
```r
#"D1_iot_wifi_sta:wifi_sta",
#"D2_iot_wifi_sta_connect:wifi_sta_connect",      
#"D3_iot_udp_client:udp_client",
"D4_iot_tcp_server:tcp_server",
#"D5_iot_mqtt:iot_mqtt",        
#"D6_iot_cloud_oc:oc_mqtt",
#"D7_iot_cloud_onenet:onenet_mqtt",
```   


### 运行结果<a name="section18115713118"></a>

示例代码编译烧录代码后，按下开发板的RESET按键，通过串口助手查看日志，会打印模块的本地IP，如本例程中的 `192.1668.0.164` ,并开始准备获取客户端的请求链接
```
g_connected: 1
netifapi_dhcp_start: 0
server :
        server_id : 192.168.0.1
        mask : 255.255.255.0, 1
        gw : 192.168.0.1
        T0 : 7200
        T1 : 3600
        T2 : 6300
clients <1> :
        mac_idx mac             addr            state   lease   tries   rto     
        0       181131a48f7a    192.168.0.164   10      0       1       2       
netifapi_netif_common: 0
start accept
```
使用 Socket tool 创建客户端用于测试。

![创建TCP_Clien](/applications/BearPi/BearPi-HM_Nano/docs/figures/D4_iot_tcp_server/创建TCP_Clien.png)

在创建客户端后点击“连接”，在数据发送窗口输入要发送的数据，点击发送后服务端会回复固定消息，且开发板收到消息后会通过日志打印出来。

```
start accept
accept addr
recv :Hello! BearPi-HM_nano TCP Server!
```

![TCP发送数据](/applications/BearPi/BearPi-HM_Nano/docs/figures/D4_iot_tcp_server/TCP发送数据.png)