#include <stdio.h>

#include <unistd.h>
#include <math.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "wifiiot_gpio.h"
#include "wifiiot_gpio_ex.h"
#include "wifiiot_errno.h"
#include "wifiiot_adc.h"
#include "sensor.h"



/***** 获取电压值函数 *****/
float GetMQ_5_Voltage(void)//瓦斯
{
    unsigned int ret;
    unsigned short data;
    float result = 0;
    ret = AdcRead(WIFI_IOT_ADC_CHANNEL_4, &data, WIFI_IOT_ADC_EQU_MODEL_8, WIFI_IOT_ADC_CUR_BAIS_DEFAULT, 0xff);//火焰

    if (ret != WIFI_IOT_SUCCESS)
    {
        printf("ADC Read Fail\n");
    }

    result = 11.5428 * 35.904 * data/4096.0*5/(25.5-5.1* data/4096.0*5);
    return pow(result,0.6549);
}

float GetMQ_7_Voltage(void)//烟雾
{
    unsigned int ret;
    unsigned short data;
    float result = 0;
    ret = AdcRead(WIFI_IOT_ADC_CHANNEL_3, &data, WIFI_IOT_ADC_EQU_MODEL_8, WIFI_IOT_ADC_CUR_BAIS_DEFAULT, 0xff);//烟雾

    if (ret != WIFI_IOT_SUCCESS)
    {
        printf("ADC Read Fail\n");
    }
    result = 11.5428 * 35.904 * data/4096.0*5/(25.5-5.1* data/4096.0*5);
    return pow(result,0.6549);
} 



float GetMQ_135Voltage(void)//氨气
{
    unsigned int ret;
    unsigned short data;

    ret = AdcRead(WIFI_IOT_ADC_CHANNEL_5, &data, WIFI_IOT_ADC_EQU_MODEL_8, WIFI_IOT_ADC_CUR_BAIS_DEFAULT, 0xff);//

    if (ret != WIFI_IOT_SUCCESS)
    {
        printf("ADC Read Fail\n");
    }

    return (float)data* 1.8 * 4 / 4096.0;
} 

float GetMQ_135Val(void){
    float ppm = 0;
    float temp = GetMQ_135Voltage();
    ppm =temp*3300/(4095*2);
	
    return ppm *1000;
//氨在室内空气中最高允许浓度为0.2635ppm
}