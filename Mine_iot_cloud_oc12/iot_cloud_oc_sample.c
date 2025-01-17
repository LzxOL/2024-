
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cmsis_os2.h"
#include "ohos_init.h"
#include "lwip/sockets.h"
#include "wifi_connect.h"
#include "E53_IA1.h"
#include "oc_mqtt.h"
// #include "watersensor.h"

#include "dht11.h"
#include "servo.h"
#include "ASR.h"
#include "oled_ssd1306.h"
#include "adc.h"
#include "aht20.h"
#include "iot_errno.h"


#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_i2c.h"
#include "iot_gpio.h"
#include "iot_pwm.h"
#include "iot_errno.h"
#include "hi_io.h"
// #include "iot_adc.h"
#include "aht20.h"
#include "oled_ssd1306.h"
#include "watersensor.h"

#include "hi_adc.h"

#define MSGQUEUE_COUNT 16 // number of Message Queue Objects
#define MSGQUEUE_SIZE 10
#define CLOUD_TASK_STACK_SIZE (1024 * 10)
#define CLOUD_TASK_PRIO 24
#define SENSOR_TASK_STACK_SIZE (1024 * 2)
#define SENSOR_TASK_PRIO 25
#define TASK_DELAY_3S 3
#define HONGAWI_TASK_PRIO 26
#define HONGAWI_TASK_STACK_SIZE (1024 * 2)
#define IDX_0          0
#define IDX_1          1
#define IDX_2          2
#define IDX_3          3

#define MS_PER_S 1000

#define BEEP_TIMES    3
#define BEEP_DURATION 100
#define BEEP_PWM_DUTY 50
#define BEEP_PWM_FREQ 4000
#define BEEP_PIN_NAME 9
#define BEEP_PIN_FUNCTION 5
#define WIFI_IOT_PWM_PORT_PWM0 0

#define GAS_SENSOR_CHAN_NAME 5
// #define GAS_SENSOR_PIN_NAME WIFI_IOT_IO_NAME_GPIO_11

#define AHT20_BAUDRATE (400 * 1000)
#define AHT20_I2C_IDX  0

#define ADC_RESOLUTION 2048
#define STACK_SIZE     4096
#define DELAY_500MS    500000
#define IDX_0          0
#define IDX_1          1
#define IDX_2          2
#define IDX_3          3
#define IDX_4          4
#define IDX_5          5
#define IDX_6          6
#define VOLTAGE_5V     (5.0)
#define EPS            (1e-7)

unsigned int g_sensorStatus = 0;
float range_20 = 20.0f;
float range_35 = 35.0f;
float range_80 = 80.0f;

extern int ret_water_bottle_2;
extern int ret_water_bottle_1;

extern int count_hongwai;

typedef struct { // object data type
    char *Buf;
    uint8_t Idx;
} MSGQUEUE_OBJ_t;

MSGQUEUE_OBJ_t msg;
osMessageQueueId_t mid_MsgQueue; // message queue id

extern int servo_flag;
// https://iot-tool.obs-website.cn-north-4.myhuaweicloud.com/

#define CLIENT_ID "668a4c2dc2e5fa1b15909df9_hi386111111_0_1_2024071004"
#define USERNAME "668a4c2dc2e5fa1b15909df9_hi386111111"
#define PASSWORD "a56c09158e4366fbdcf5e1b93aad601caf0cdb9781a5e1f55132e08a120f12d4"


// 设备ID
// 6635a27571d845632a08469b_20240504
// 设备密钥
// e8045b4826894e7b9d6cbc5ba2bcb553


typedef enum {
    en_msg_cmd = 0,
    en_msg_report,
} en_msg_type_t;

typedef struct {
    char *request_id;
    char *payload;
} cmd_t;

typedef struct {
    int lum;
    int temp;
    int hum;
    int hongwai;//红外
    int shuiwei;//水位

    int temp_flag;
    int hum_flag;
    int water_flag;

} report_t;

typedef struct {
    en_msg_type_t msg_type;
    union {
        cmd_t cmd;
        report_t report;
    } msg;
} app_msg_t;

typedef struct {
    int connected;
    int led;
    int motor;
} app_cb_t;
static app_cb_t g_app_cb;
report_t report_temp;

//发送至MQTT
static void ReportMsg(report_t *report)
{
    oc_mqtt_profile_service_t service;
    oc_mqtt_profile_kv_t temperature;
    oc_mqtt_profile_kv_t humidity;
    oc_mqtt_profile_kv_t luminance;
    oc_mqtt_profile_kv_t led;
    oc_mqtt_profile_kv_t motor;

    oc_mqtt_profile_kv_t hongwai_oc;
    oc_mqtt_profile_kv_t servo_cmd_oc;

    oc_mqtt_profile_kv_t temp_beep;
    oc_mqtt_profile_kv_t hum_beep;
    oc_mqtt_profile_kv_t water_beep;

    // service.event_time = NULL;
    // service.service_id = "Agriculture";
    // service.service_property = &temperature;
    // service.nxt = NULL;

    // temperature.key = "Temperature";
    // temperature.value = &report->temp;
    // temperature.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    // temperature.nxt = &humidity;

    // humidity.key = "Humidity";
    // humidity.value = &report->hum;
    // humidity.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    // humidity.nxt = &luminance;

    // luminance.key = "Luminance";
    // luminance.value = &report->lum;
    // luminance.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    // luminance.nxt = &hongwai_oc; //下一个变量

    service.event_time = NULL;
    service.service_id = "Agriculture";
    service.service_property = &temperature;
    service.nxt = NULL;

    temperature.key = "Temperature";
    temperature.value = &report->temp;
    temperature.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    temperature.nxt = &humidity;

    humidity.key = "Humidity";
    humidity.value = &report->hum;
    humidity.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    humidity.nxt = &temp_beep;

    temp_beep.key = "Temp_Flag";//滴液检测次数
    temp_beep.value = &report->temp_flag;
    temp_beep.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    temp_beep.nxt = &hum_beep; //下一个变量为空

    hum_beep.key = "Hum_flag";//滴液检测次数
    hum_beep.value = &report->hum_flag;
    hum_beep.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    hum_beep.nxt = &water_beep; //下一个变量为空
    
    water_beep.key = "Water_flag";//滴液检测次数
    water_beep.value = &report->water_flag;
    water_beep.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    water_beep.nxt = NULL; //下一个变量为空
    
    // servo_cmd_oc.key = "Servo_Cmd";//滴液检测次数
    // servo_cmd_oc.value = &report->hongwai;
    // servo_cmd_oc.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    // servo_cmd_oc.nxt = NULL; //下一个变量为空
     oc_mqtt_profile_propertyreport(USERNAME, &service);
    return;
}

/**
 * @brief 消息接收回调函数
 *
 * 当接收到消息时，此回调函数将被调用。它接收接收到的数据，并将数据放入消息队列中。
 *
 * @param recv_data 接收到的数据指针
 * @param recv_size 接收到的数据大小
 * @param resp_data 响应数据指针（输出）
 * @param resp_size 响应数据大小（输出）
 */
void MsgRcvCallback(uint8_t *recv_data, size_t recv_size, uint8_t **resp_data, size_t *resp_size)
{
    app_msg_t *app_msg;

    int ret = 0;
    app_msg = malloc(sizeof(app_msg_t));
    app_msg->msg_type = en_msg_cmd;
    app_msg->msg.cmd.payload = (char *)recv_data;

    printf("recv data is %s\n", recv_data);
    ret = osMessageQueuePut(mid_MsgQueue, &app_msg, 0U, 0U);
    if (ret != 0) {
        free(recv_data);
    }
    *resp_data = NULL;
    *resp_size = 0;
}

static void oc_cmdresp(cmd_t *cmd, int cmdret)
{
    oc_mqtt_profile_cmdresp_t cmdresp;
    ///< do the response
    cmdresp.paras = NULL;
    cmdresp.request_id = cmd->request_id;
    cmdresp.ret_code = cmdret;
    cmdresp.ret_name = NULL;
    (void)oc_mqtt_profile_cmdresp(NULL, &cmdresp);
}
///< COMMAND DEAL
#include <cJSON.h>
static void DealCmdMsg(cmd_t *cmd)
{
    cJSON *obj_root, *obj_cmdname, *obj_paras, *obj_para;

    int cmdret = 1;

    obj_root = cJSON_Parse(cmd->payload);
    if (obj_root == NULL) {
        oc_cmdresp(cmd, cmdret);
    }

    obj_cmdname = cJSON_GetObjectItem(obj_root, "command_name");
    if (obj_cmdname == NULL) {
        cJSON_Delete(obj_root);
    }
    if (strcmp(cJSON_GetStringValue(obj_cmdname), "Agriculture_Control_light") == 0) {
        obj_paras = cJSON_GetObjectItem(obj_root, "paras");
        if (obj_paras == NULL) {
            cJSON_Delete(obj_root);
        }
        obj_para = cJSON_GetObjectItem(obj_paras, "Light");
        if (obj_para == NULL) {
            cJSON_Delete(obj_root);
        }
        ///< operate the LED here
        if (strcmp(cJSON_GetStringValue(obj_para), "ON") == 0) {
            g_app_cb.led = 1;
            LightStatusSet(ON);
            printf("Light On!");
        } else {
            g_app_cb.led = 0;
            LightStatusSet(OFF);
            printf("Light Off!");
        }
        cmdret = 0;
    } else if (strcmp(cJSON_GetStringValue(obj_cmdname), "Agriculture_Control_Motor") == 0) {//舵机控制
        obj_paras = cJSON_GetObjectItem(obj_root, "Paras");
        if (obj_paras == NULL) {
            cJSON_Delete(obj_root);
        }
        obj_para = cJSON_GetObjectItem(obj_paras, "Motor");
        if (obj_para == NULL) {
            cJSON_Delete(obj_root);
        }
        ///< operate the Motor here
        if (strcmp(cJSON_GetStringValue(obj_para), "ON") == 0) {
            servo_flag = 1;
            // MotorStatusSet(ON); 
            // EngineTurnLeft();
            // TaskMsleep(200);
            printf("Bottle 1");

        } else if (strcmp(cJSON_GetStringValue(obj_para), "OFF") == 0){
            servo_flag = 2;
            // MotorStatusSet(OFF);
            // EngineTurnRight();
            // TaskMsleep(200);
            printf("Bottle 2");
        }
        else {
            servo_flag = 3;
            // MotorStatusSet(OFF);
            // EngineTurnRight();
            // TaskMsleep(200);
            printf("Bottle 3");
        }
        cmdret = 0;
    }

    cJSON_Delete(obj_root);
}

//连接wifi
static int CloudMainTaskEntry(void)
{
    app_msg_t *app_msg;

    uint32_t ret = WifiConnect("H", "123456789");

    device_info_init(CLIENT_ID, USERNAME, PASSWORD);
    oc_mqtt_init();
    oc_set_cmd_rsp_cb(MsgRcvCallback);

    while (1) {
        app_msg = NULL;
        (void)osMessageQueueGet(mid_MsgQueue, (void **)&app_msg, NULL, 1);
        if (app_msg != NULL) {
            switch (app_msg->msg_type) {
                case en_msg_cmd:
                    DealCmdMsg(&app_msg->msg.cmd);
                    break;
                case en_msg_report:
                    ReportMsg(&app_msg->msg.report);
                    break;
                default:
                    break;
            }
            free(app_msg);
        }
    }
    return 0;
}

static int Temperature_SensorTaskEntry(void)
{
    // OledInit();
    // OledFillScreen(0);
    static char line[32] = {0};
    while(DHT11_Init())	//DHT11初始化	
	{
		printf("DHT11 Init Error!!\r\n");
 		usleep(100000);
	}		
    printf("DHT11 Init Successful!!");

    int ret;
    app_msg_t *app_msg;
    int temp=0;  	    
    int humi=0;
    int ledflag =0;
    uint32_t retval = 0;
    // set BEEP pin as PWM function 蜂鸣器初始化

    IoTGpioInit(BEEP_PIN_NAME);
    retval = hi_io_set_func(BEEP_PIN_NAME, BEEP_PIN_FUNCTION);
    if (retval != IOT_SUCCESS) {
        printf("IoTGpioInit(9) failed, %0X!\n", retval);
    }
    IoTGpioSetDir(BEEP_PIN_NAME, IOT_GPIO_DIR_OUT);
    IoTPwmInit(WIFI_IOT_PWM_PORT_PWM0);

    for (int i = 0; i < BEEP_TIMES; i++) {
        // ret = snprintf(line, sizeof(line), "beep %d/%d", (i + 1), BEEP_TIMES);
        ret = snprintf_s(line, sizeof(line), sizeof(line) - 1, "beep %d/%d", (i + 1), BEEP_TIMES);
        if (ret < 0) {
            continue;
        }

        OledShowString(0, IDX_0, line, 1);

        IoTPwmStart(WIFI_IOT_PWM_PORT_PWM0, BEEP_PWM_DUTY, BEEP_PWM_FREQ);
        usleep(BEEP_DURATION * MS_PER_S);
        IoTPwmStop(WIFI_IOT_PWM_PORT_PWM0);
        usleep((MS_PER_S - BEEP_DURATION) * MS_PER_S);
    }

    while (1) {

        if(DHT11_Read_Data(&temp,&humi)==0)	//读取温湿度值
        {   
          if((temp!= 0)||(humi!=0))
          {
            ledflag++;
            printf("Temperature = %d\r\n",temp);
            printf("Humidity = %d%%\r\n",humi);
          }
        }

        

        //oled显示
        OledShowString(0, IDX_0, "Sensor values:", 1);
        // ret = snprintf(line, sizeof(line), "temp: %.2f", temperature);
        ret = snprintf_s(line, sizeof(line), sizeof(line) - 1, "temp: %d", temp);
        if (ret < 0) {
            continue;
        }
        OledShowString(0, IDX_1, line, 1);
        // ret = snprintf(line, sizeof(line), "humi: %.2f", humidity);
        ret = snprintf_s(line, sizeof(line), sizeof(line) - 1, "humi: %d", humi);
        if (ret < 0) {
            continue;
        }
        OledShowString(0, IDX_2, line, 1);

        if (temp > range_35 || temp < 0) {
            g_sensorStatus++;
            report_temp.temp_flag = 1;
            OledShowString(0, IDX_5, "temp abnormal!!", 1);
        }
        else report_temp.temp_flag = 0;

        if (humi < range_20 || humi > range_80) {
            g_sensorStatus++;
            report_temp.hum_flag = 1;
            if (temp > range_35 || temp < 0) {
                OledShowString(0, IDX_6, "humi abnormal!!", 1);
            } else {
                OledShowString(0, IDX_5, "humi abnormal!!", 1);
            }
        }
        else report_temp.hum_flag = 0;

        if (g_sensorStatus > 0) {
            IoTPwmStart(WIFI_IOT_PWM_PORT_PWM0, BEEP_PWM_DUTY, BEEP_PWM_FREQ);
            usleep(DELAY_500MS);
            IoTPwmStop(WIFI_IOT_PWM_PORT_PWM0);
            g_sensorStatus = 0;
        }
        usleep(1000000);
        app_msg = malloc(sizeof(app_msg_t));

        printf("temp:%d hum:%d\r\n", temp, humi);
        if (app_msg != NULL) {
            app_msg->msg_type = en_msg_report;
            app_msg->msg.report.hum = (int)humi;
            // app_msg->msg.report.lum = temp;
            app_msg->msg.report.temp = (int)temp;
            app_msg->msg.report.temp_flag = report_temp.temp_flag;
            app_msg->msg.report.hum_flag = report_temp.hum_flag;
            printf("temp_flag = %d\r\n", app_msg->msg.report.temp_flag);
            printf("hum_flag = %d\r\n", app_msg->msg.report.hum_flag);

            if (osMessageQueuePut(mid_MsgQueue, &app_msg, 0U, 0U) != 0) {
                free(app_msg);
            }
        }
        sleep(1);
    }
    return 0;
}


// static int  Water_SenorInit(void)
// {
//     int ret;
//     app_msg_t *app_msg;
//     static char line[32] = {0};
// while (1)
// {
//     int ret;
//     app_msg_t *app_msg;
//     static char line[32] = {0};
//      void AdcGpioTask(void);

//         app_msg = malloc(sizeof(app_msg_t));
//         if (app_msg != NULL) {
//             app_msg->msg_type = en_msg_report;
//             app_msg->msg.report.shuiwei = value1;
//             printf("APP_MSG!!!!!!!!!!!!!!\r\n");
//             printf("MQTT Hongwai Count:%d\r\n",value1);
//              if (osMessageQueuePut(mid_MsgQueue, &app_msg, 0U, 0U) != 0) {
//                 free(app_msg);
//             }
//         } 
//          ret = snprintf_s(line, sizeof(line), sizeof(line) - 1, "yuliang: %d ", value1);
//         if (ret < 0) {
//             continue;
//         }
//         OledShowString(0, 4, line, 1)

// }

   




// static int Servo_SensorTaskEntry(void)
// {   
//     app_msg_t *app_msg;
//     while (1) {
//         S92RInit();
//         app_msg = malloc(sizeof(app_msg_t));


//         if (app_msg != NULL) {
//             app_msg->msg_type = en_msg_report;
//             app_msg->msg.report.servo_cmd = count_hongwai;
//             printf("APP_MSG!!!!!!!!!!!!!!\r\n");
//             printf("MQTT Hongwai Count:%d\r\n",count_hongwai);

//             if (osMessageQueuePut(mid_MsgQueue, &app_msg, 0U, 0U) != 0) {
//                 free(app_msg);
//             }
//         }
//         sleep(TASK_DELAY_3S);
//     }
//     return 0;
// }
static int Water_SensorTaskEntry(void)
{  
    
    int ret;
    app_msg_t *app_msg;
    static char line[32] = {0};
    Water_SenorInit();
    static int count = 10;
    int water_warning_flag =0;
    uint32_t retval;
    IoTI2cInit(AHT20_I2C_IDX, AHT20_BAUDRATE);

    // set BEEP pin as PWM function
    IoTGpioInit(BEEP_PIN_NAME);
    retval = hi_io_set_func(BEEP_PIN_NAME, BEEP_PIN_FUNCTION);
    if (retval != IOT_SUCCESS) {
        printf("IoTGpioInit(9) failed, %0X!\n", retval);
    }
    IoTGpioSetDir(BEEP_PIN_NAME, IOT_GPIO_DIR_OUT);
    IoTPwmInit(WIFI_IOT_PWM_PORT_PWM0);

    for (int i = 0; i < BEEP_TIMES; i++) {
        // ret = snprintf(line, sizeof(line), "beep %d/%d", (i + 1), BEEP_TIMES);
        ret = snprintf_s(line, sizeof(line), sizeof(line) - 1, "beep %d/%d", (i + 1), BEEP_TIMES);
        if (ret < 0) {
            continue;
        }

        OledShowString(0, IDX_0, line, 1);

        IoTPwmStart(WIFI_IOT_PWM_PORT_PWM0, BEEP_PWM_DUTY, BEEP_PWM_FREQ);
        usleep(BEEP_DURATION * MS_PER_S);
        IoTPwmStop(WIFI_IOT_PWM_PORT_PWM0);
        usleep((MS_PER_S - BEEP_DURATION) * MS_PER_S);
    }


    while (1) {
        if(ret_water_bottle_1 || ret_water_bottle_2){
            water_warning_flag = 1;
        }
        app_msg = malloc(sizeof(app_msg_t));
        if (app_msg != NULL) {
            app_msg->msg_type = en_msg_report;
            app_msg->msg.report.water_flag = water_warning_flag;
           // printf("APP_MSG!!!!!!!!!!!!!!\r\n");
            printf("Water_level_1:%d\r\n",ret_water_bottle_1);
            printf("Water_level_2:%d\r\n",ret_water_bottle_2);

             if (osMessageQueuePut(mid_MsgQueue, &app_msg, 0U, 0U) != 0) {
                free(app_msg);
            }
        } 
        if (ret_water_bottle_1 && count)
        {   
            count --;
            IoTPwmStart(WIFI_IOT_PWM_PORT_PWM0, BEEP_PWM_DUTY, BEEP_PWM_FREQ);
            usleep(DELAY_500MS);
            IoTPwmStop(WIFI_IOT_PWM_PORT_PWM0);
            
        }
        else ret_water_bottle_1 = 0;
        if (ret_water_bottle_2 && count)
        {   
            count --;
            IoTPwmStart(WIFI_IOT_PWM_PORT_PWM0, BEEP_PWM_DUTY, BEEP_PWM_FREQ);
            usleep(DELAY_500MS);
            IoTPwmStop(WIFI_IOT_PWM_PORT_PWM0);
            
        }
        else ret_water_bottle_2 = 0;
        // //oled显示
        // ret = snprintf_s(line, sizeof(line), sizeof(line) - 1, "speed: %d ", count_hongwai);
        // if (ret < 0) {
        //     continue;
        // }
        // OledShowString(0, IDX_3, line, 1);
        sleep(1);
    }
    return 0;
}



static void IotMainTaskEntry(void)
{
    
    mid_MsgQueue = osMessageQueueNew(MSGQUEUE_COUNT, MSGQUEUE_SIZE, NULL);
    if (mid_MsgQueue == NULL) {
        printf("Failed to create Message Queue!\n");
    }
    OledInit();
    OledFillScreen(0);

    osThreadAttr_t attr;

    // attr.name = "CloudMainTaskEntry";
    // attr.attr_bits = 0U;
    // attr.cb_mem = NULL;
    // attr.cb_size = 0U;
    // attr.stack_mem = NULL;
    // attr.stack_size = CLOUD_TASK_STACK_SIZE;
    // attr.priority = CLOUD_TASK_PRIO;

    // if (osThreadNew((osThreadFunc_t)CloudMainTaskEntry, NULL, &attr) == NULL) {
    //     printf("Failed to create CloudMainTaskEntry!\n");
    // }

    // //舵机控制线程
    attr.name = "Servo_Task";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 1024*4; // 堆栈大小为1024 stack size 1024
    attr.priority = osPriorityNormal;
    if (osThreadNew((osThreadFunc_t)Sg92RTask, NULL, &attr) == NULL) {
        printf("[LedExample] Failed to create LedTask!\n");
    }

    // //温湿度线程
    IoTGpioInit(BEEP_PIN_NAME);
    hi_io_set_func(BEEP_PIN_NAME, BEEP_PIN_FUNCTION);
    IoTPwmInit(WIFI_IOT_PWM_PORT_PWM0);

    // attr.name = "EnvironmentTask";
    // attr.attr_bits = 0U;
    // attr.cb_mem = NULL;
    // attr.cb_size = 0U;
    // attr.stack_mem = NULL;
    // attr.stack_size = STACK_SIZE;
    // attr.priority = osPriorityNormal;

    // if (osThreadNew((osThreadFunc_t)EnvironmentTask, NULL, &attr) == NULL) {
    //     printf("[EnvironmentDemo] Falied to create EnvironmentTask!\n");
    // }

    // //温湿度线程
    attr.name = "DHT11_Task";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 1024*5 ;
    attr.priority = 25;

    if (osThreadNew((osThreadFunc_t)Temperature_SensorTaskEntry, NULL, &attr) == NULL)
    {
        printf("Falied to create DHT11_Task!\n");
    }

    // //水位读取线程
    attr.name = "Water_SenorInit";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 10240;
    attr.priority = 25;
    if (osThreadNew((osThreadFunc_t)Water_SensorTaskEntry, NULL, &attr) == NULL)
    {
        printf("Falied to create WifiAPTask!\r\n");
    }

    //串口
    // attr.name = "UartTask";
    // attr.attr_bits = 0U;
    // attr.cb_mem = NULL;
    // attr.cb_size = 0U;
    // attr.stack_mem = NULL;
    // attr.stack_size = 1024 * 8;
    // attr.priority = 25;

    // if (osThreadNew((osThreadFunc_t)UartTask, NULL, &attr) == NULL) {
    //     printf("Failed to create UartTask!\n");
    // }
    

    // attr.name = "UDPServerTask";
    // attr.attr_bits = 0U;
    // attr.cb_mem = NULL;
    // attr.cb_size = 0U;
    // attr.stack_mem = NULL;
    // attr.stack_size = 10240;
    // attr.priority = osPriorityNormal;

    // if (osThreadNew((osThreadFunc_t)UDPServerTask, NULL, &attr) == NULL) {
    //     printf("[TCPServerDemo] Failed to create TCPServerTask!\n");
    // }
}

APP_FEATURE_INIT(IotMainTaskEntry);