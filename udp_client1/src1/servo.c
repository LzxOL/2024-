/*
 * Copyright (c) 2022 HiSilicon (Shanghai) Technologies CO., LIMITED.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>

#include "ohos_init.h"
#include "cmsis_os2.h"

#include "hi_io.h"


#include "hi_time.h"
#include "iot_gpio_ex.h"
#include "iot_gpio.h"


#define  COUNT   10
#define  FREQ_TIME    20000
int servo_flag = 0;

void SetAngle(unsigned int duty)
{
    unsigned int time = FREQ_TIME;

    IoTGpioSetOutputVal(IOT_IO_NAME_GPIO_2,1)  ;  
    hi_udelay(duty);
    IoTGpioSetOutputVal(IOT_IO_NAME_GPIO_2,0)  ;  
    hi_udelay(time - duty);
}


/* The steering gear is centered
 * 1、依据角度与脉冲的关系，设置高电平时间为1500微秒
 * 2、不断地发送信号，控制舵机居中
*/


/* Turn 90 degrees to the right of the steering gear
 * 1、依据角度与脉冲的关系，设置高电平时间为500微秒
 * 2、不断地发送信号，控制舵机向右旋转90度
*/
/*  Steering gear turn right */
void EngineTurnRight(void)
{
    unsigned int angle = 500;
    for (int i = 0; i < COUNT; i++) {
        SetAngle(angle);
    }
}


/* Turn 90 degrees to the left of the steering gear
 * 1、依据角度与脉冲的关系，设置高电平时间为2500微秒
 * 2、不断地发送信号，控制舵机向左旋转90度
*/
/* Steering gear turn left */
void EngineTurnLeft(void)
{
    unsigned int angle = 2500;
    for (int i = 0; i < COUNT; i++) {
        SetAngle(angle);
    }
}

void stop_trans(void){
    unsigned int angle = 2500;
    for (int i = 0; i < COUNT; i++) {
        SetAngle(angle);
    }
}

void half_stop_trans(void){
    unsigned int angle = 1850;
    for (int i = 0; i < COUNT; i++) {
        SetAngle(angle);
    }
}

void RegressMiddle(void)
{
    unsigned int angle = 1500;
    for (int i = 0; i < COUNT; i++) {
        SetAngle(angle);

    }
}

void S92RInit(void)
{
  //控制输液速度
  IoTGpioInit(IOT_IO_NAME_GPIO_2); //初始化GPIO
  IoSetFunc(IOT_IO_NAME_GPIO_2, IOT_IO_FUNC_GPIO_2_GPIO);//设置GPIO_2的复用功能为普通GPIO
  IoTGpioSetDir(IOT_IO_NAME_GPIO_2, IOT_GPIO_DIR_OUT);//设置GPIO_2为输出模式
}

void Sg92RTask(void)
{
    //unsigned int time = 200;
    S92RInit();
    unsigned int time = 200;
    RegressMiddle();
     
    while (1) {
        // if(servo_flag == 1){
        //     RegressMiddle();
        //     TaskMsleep(time);
        // }
        // else if (servo_flag == 2){
        //     stop_trans();
        //     TaskMsleep(time);
        // }
        // else if (servo_flag == 3){
        //     half_stop_trans();
        //     TaskMsleep(time);
        // }
        if(servo_flag == 1){
            RegressMiddle();
            TaskMsleep(time);
         }
         else if (servo_flag == 2){
            stop_trans();
            TaskMsleep(time);
         }
         else if (servo_flag == 3){
            half_stop_trans();
            TaskMsleep(time);
         }


        // //如果servo_flag == 1;输液针拔开，同时切换输液袋一次

        // /* 舵机归中 Steering gear centering */

        // RegressMiddle();
        // TaskMsleep(time);

        // // /*
        // //  * 舵机左转90度
        // //  * Steering gear turns 90 degrees to the left
        // //  */
        // EngineTurnLeft();
        // TaskMsleep(time);
        // /*
        //  * 舵机右转90度
        //  * Steering gear turns right 90 degrees
        //  */
        // EngineTurnRight();
        // TaskMsleep(time);

        // /* 舵机归中 Steering gear centering */
        // RegressMiddle();
        // TaskMsleep(time);

    }
}

// void Sg92rSampleEntry(void)
// {
//     osThreadAttr_t attr;
//    // IoTWatchDogDisable();
//     attr.name = "Sg92RTask";
//     attr.attr_bits = 0U;
//     attr.cb_mem = NULL;
//     attr.cb_size = 0U;
//     attr.stack_mem = NULL;
//     attr.stack_size = 1024 * 5; // 堆栈大小为1024*5 stack size 5*1024
//     attr.priority = osPriorityNormal;

//     if (osThreadNew((osThreadFunc_t)Sg92RTask, NULL, &attr) == NULL) {
//         printf("[GA12N205Task] Failed to create Hcsr04SampleTask!\n");
//     }
// }

// APP_FEATURE_INIT(Sg92rSampleEntry);