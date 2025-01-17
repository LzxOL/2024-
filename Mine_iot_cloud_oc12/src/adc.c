#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_gpio_ex.h"
#include "iot_gpio.h"
#include "iot_adc.h"
#include "iot_errno.h"

#define ADC_TASK_STACK_SIZE 1024 * 8
#define ADC_TASK_PRIO 24

/***** 获取电压值函数 *****/
static float GetVoltage(void)
{
    unsigned int ret;
    unsigned short data;

    // 该函数通过使用AdcRead()函数来读取 ADC_CHANNEL_5 的数值存储在data中， 
    // WIFI_IOT_ADC_EQU_MODEL_8 表示8次平均算法模式，
    // WIFI_IOT_ADC_CUR_BAIS_DEFAULT 表示默认的自动识别模式，
    // 最后通过 data * 1.8 * 4 / 4096.0 计算出实际的电压值。
    ret = IoTAdcRead(IOT_ADC_EQU_MODEL_8, &data, IOT_ADC_EQU_MODEL_8, IOT_ADC_CUR_BAIS_DEFAULT, 0xff);
    if (ret != IOT_SUCCESS)
    {
        printf("ADC Read Fail\n");
    }

    return (float)data * 1.8 * 4 / 4096.0;
}

void ADCTask(void)
{
    float voltage;

    //上拉，让按键未按下时GPIO_11保持高电平状态
    IoSetPull(IOT_IO_NAME_GPIO_11, IOT_IO_PULL_UP);
    while (1)
    {
        printf("=======================================\r\n");
        printf("***************ADC_example*************\r\n");
        printf("=======================================\r\n");

        //获取电压值
        voltage = GetVoltage();
        printf("vlt:%.3fV\n", voltage);

        //延时1s
        usleep(1000000);
    }
}

// static void ADCExampleEntry(void)
// {
//     osThreadAttr_t attr;

//     attr.name = "ADCTask";
//     attr.attr_bits = 0U;
//     attr.cb_mem = NULL;
//     attr.cb_size = 0U;
//     attr.stack_mem = NULL;
//     attr.stack_size = ADC_TASK_STACK_SIZE;
//     attr.priority = ADC_TASK_PRIO;

//     if (osThreadNew((osThreadFunc_t)ADCTask, NULL, &attr) == NULL)
//     {
//         printf("[ADCExample] Falied to create ADCTask!\n");
//     }
// }

// APP_FEATURE_INIT(ADCExampleEntry);
