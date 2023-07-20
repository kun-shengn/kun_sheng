/*
 * Copyright (c) 2020 Nanjing Xiaoxiongpai Intelligent Technology Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include "ohos_init.h"
#include "cmsis_os2.h"

#include "sensor.h"
#include "DHT11.h"
#include "wifi_device.h"
#include "lwip/netifapi.h"
#include "lwip/api_shell.h"
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include "lwip/sockets.h"
#include "wifi_connect.h"
#include "wifiiot_gpio.h"
#include "wifiiot_gpio_ex.h"
#include "wifiiot_adc.h"
#include "wifiiot_uart.h"
#include "wifiiot_errno.h"

#define _PROT_ 1111
#define UART_TASK_STACK_SIZE 1024 * 8
#define UART_TASK_PRIO 25
#define UART_BUFF_SIZE 1000

//在sock_fd 进行监听，在 new_fd 接收新的链接
int sock_fd;
int Humidity,Temperature;//温湿度标签
int addr_length;
char uart_check[] = {0xC7,0xEB,0xD5,0xFD,0xC8,0xB7,0xC5,0xE5,0xB4,0xF7,0xCD,0xB7,0xBF,0xF8 };
char uart_CO[] = {0xB5,0xB1,0xC7,0xB0 ,0xBC,0xD7 ,0xCD,0xE9 ,0xC5,0xA8 ,0xB6,0xC8 ,0xB3,0xAC ,0xB1,0xEA ,0xA3,0xAC ,0xC7,0xEB ,0xBE,0xA1 ,0xBF,0xEC ,0xB3,0xB7 ,0xC0,0xEB };
static char send_data[100] = {0};
float mq5 = 0,mq7 = 0;

static void CheckTask(void);
static void CheckHandler(void);
static void SensorTask(void);
static void SensorHandler(void);

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
    //CheckHandler();
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

static void CheckTask(void)
{
    WifiIotGpioValue checkinfo={0};
    while(1)
    {
        GpioGetInputVal(WIFI_IOT_GPIO_IDX_11,&checkinfo);
        if(checkinfo == WIFI_IOT_GPIO_VALUE1)
        {
            UartWrite(WIFI_IOT_UART_IDX_1, (unsigned char *)uart_check, strlen(uart_check)); 
            sleep(5);
        }
    }
    
}

static void CheckHandler(void)
{
    osThreadAttr_t attr;
    attr.name = "CheckTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 2048;
    attr.priority = 26;
    if (osThreadNew((osThreadFunc_t)CheckTask, NULL, &attr) == NULL) {
        printf("Check:create task fail!\r\n");
    }
}

static void SensorTask(void)
{
    while(1)
    {
        DHT11ReadData(&Humidity, &Temperature);
        printf("H:%d,T:%d",Humidity, Temperature);
        mq5 = GetMQ_5_Voltage();
        mq7 = GetMQ_7_Voltage();
        DHT11ReadData(&Humidity, &Temperature);
        printf("mq5:%f,mq7:%f\n",mq5, mq7);
        sleep(1);
        if(mq5>10)
        {
            UartWrite(WIFI_IOT_UART_IDX_1, (unsigned char *)uart_CO, strlen(uart_CO)); 
            sleep(5);
        }
        sprintf(send_data,"H:%dT:%dmq5:%dmq7:%d", Humidity, Temperature,(int)mq5,(int)mq7);
        sleep(1);
        //UartWrite(WIFI_IOT_UART_IDX_1, (unsigned char *)uart_buff, strlen(uart_buff)); 
    }
}

static void SensorHandler(void)
{
    osThreadAttr_t attr;
    attr.name = "SensorTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 2048;
    attr.priority = 27;
    if (osThreadNew((osThreadFunc_t)SensorTask, NULL, &attr) == NULL) {
        printf("Sensor:create task fail!\r\n");
    }
}

static void SecurityClient(void)
{
    osThreadAttr_t attr;

    attr.name = "TCPClientTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 10240;
    attr.priority = 25;

    if (osThreadNew((osThreadFunc_t)TCPClientTask, NULL, &attr) == NULL)
    {
        printf("[TCPClientDemo] Falied to create TCPClientTask!\n");
    }
}

APP_FEATURE_INIT(SecurityClient);
