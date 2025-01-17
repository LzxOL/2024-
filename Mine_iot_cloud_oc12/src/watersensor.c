#include "dht11.h"
#include "hi_time.h"
#include "ohos_init.h"
#include <stdio.h>
#include "iot_gpio_ex.h"
#include "iot_gpio.h"
#include <stdio.h>
#include <unistd.h>

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "watersensor.h"
#include "iot_gpio_ex.h"
#include "iot_gpio.h"

int ret_water_bottle_1 = 0;
int ret_water_bottle_2 = 0;

void sensor_Init_bottle1(char *arg){
    (void) arg;
    ret_water_bottle_1 = 1; 
    printf("The Water_Bottle_1 is missing:%d\r\n", ret_water_bottle_1);//置1就说明水位下降
}

void sensor_Init_bottle2(char *arg){
    (void) arg;
    ret_water_bottle_2 = 1; 
    printf("The Water_Bottle_1 is missing:%d\r\n", ret_water_bottle_2);//置1就说明水位下降
}

void Water_SenorInit(void)
{   
    printf("Start Init!!!\n");
    // LED3的GPIO初始化 GPIO initialization of LED3
    IoTGpioInit(IOT_IO_NAME_GPIO_11);
    // 设置GPIO9的管脚复用关系为GPIO Set the pin reuse relationship of GPIO9 to GPIO
    IoSetFunc(IOT_IO_NAME_GPIO_11, IOT_IO_FUNC_GPIO_11_GPIO);
    // GPIO方向设置为输出 GPIO direction set to output
    IoTGpioSetDir(IOT_IO_NAME_GPIO_11, IOT_GPIO_DIR_IN);
    IoSetPull(IOT_IO_NAME_GPIO_11,IOT_IO_PULL_UP);
    IoTGpioRegisterIsrFunc(IOT_IO_NAME_GPIO_11,IOT_INT_TYPE_EDGE,IOT_GPIO_EDGE_RISE_LEVEL_HIGH,sensor_Init_bottle1,NULL);

    // LED3的GPIO初始化 GPIO initialization of LED3
    IoTGpioInit(IOT_IO_NAME_GPIO_12);
    // 设置GPIO9的管脚复用关系为GPIO Set the pin reuse relationship of GPIO9 to GPIO
    IoSetFunc(IOT_IO_NAME_GPIO_12, IOT_IO_FUNC_GPIO_12_GPIO);
    // GPIO方向设置为输出 GPIO direction set to output
    IoTGpioSetDir(IOT_IO_NAME_GPIO_12, IOT_GPIO_DIR_IN);
    IoSetPull(IOT_IO_NAME_GPIO_12,IOT_IO_PULL_UP);
    IoTGpioRegisterIsrFunc(IOT_IO_NAME_GPIO_12,IOT_INT_TYPE_EDGE,IOT_GPIO_EDGE_RISE_LEVEL_HIGH,sensor_Init_bottle2,NULL);
}
