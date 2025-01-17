
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
#include "oc_mqtt.h"

#include "servo.h"
#include "udp_client_demo.h"
#include "hongwai.h"
#include "oled_ssd1306.h"


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

extern int count_hongwai;
extern int servo_flag;

typedef struct { // object data type
    char *Buf;
    uint8_t Idx;
} MSGQUEUE_OBJ_t;

MSGQUEUE_OBJ_t msg;
osMessageQueueId_t mid_MsgQueue; // message queue id


// https://iot-tool.obs-website.cn-north-4.myhuaweicloud.com/

#define CLIENT_ID "668d842f752c794e18cda6f7_derkbanz1_0_1_2024071004"
#define USERNAME "668d842f752c794e18cda6f7_derkbanz1"
#define PASSWORD "a56c09158e4366fbdcf5e1b93aad601caf0cdb9781a5e1f55132e08a120f12d4"


// 设备ID
// 6635a27571d845632a08469b_20240504
// 设备密钥
// e8045b4826894e7b9d6cbc5ba2bcb553

int watersensor_flag=0;

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
    int servo_cmd;
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

    service.event_time = NULL;
    service.service_id = "HONGWAI";
    service.service_property = &hongwai_oc;
    service.nxt = NULL;

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

    hongwai_oc.key = "Hongwai_Count";
    hongwai_oc.value = &report->hongwai;
    hongwai_oc.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    hongwai_oc.nxt = NULL;

    oc_mqtt_profile_propertyreport(USERNAME, &service);
    return;
}

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
            // LightStatusSet(ON);
            printf("Light On!");
        } else {
            g_app_cb.led = 0;
            // LightStatusSet(OFF);
            printf("Light Off!");
        }
        cmdret = 0;
    } else if (strcmp(cJSON_GetStringValue(obj_cmdname), "Agriculture_Control_Motor111") == 0) {//舵机控制
        obj_paras = cJSON_GetObjectItem(obj_root, "Paras");
        if (obj_paras == NULL) {
            cJSON_Delete(obj_root);
        }
        obj_para = cJSON_GetObjectItem(obj_paras, "Motor1");
        if (obj_para == NULL) {
            cJSON_Delete(obj_root);
        }
        ///< operate the Motor here
        if (strcmp(cJSON_GetStringValue(obj_para), "ON") == 0) {
            servo_flag = 1;
            // MotorStatusSet(ON); 
            // EngineTurnLeft();
            // TaskMsleep(200);
            printf("Servo Open");

        } else if (strcmp(cJSON_GetStringValue(obj_para), "OFF") == 0){
            servo_flag = 2;
            // MotorStatusSet(OFF);
            // EngineTurnRight();
            // TaskMsleep(200);
            printf("Servo Close");
        }
        else {
            servo_flag = 3;
            // MotorStatusSet(OFF);
            // EngineTurnRight();
            // TaskMsleep(200);
            printf("Servo Half");
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

static int HONGWAI_SensorTaskEntry(void)
{  
    
    int ret;
    app_msg_t *app_msg;
    static char line[32] = {0};
    Hongwai_IO_Init();
    int count_min = 0;
    int count_speed = 0;
    while (1) {
        count_min ++;
        if(count_min >=3){
            count_min = 0;
            count_speed = count_hongwai*20;//1滴0.05ml
            printf("Speed:%dml/min\r\n",count_speed);
            count_hongwai = 0;
             OledShowString(0, 0, "speed:", 1);
        // ret = snprintf(line, sizeof(line), "temp: %.2f", temperature);
        ret = snprintf_s(line, sizeof(line), sizeof(line) - 1, " %d", count_speed);
        if (ret < 0) {
            continue;
        }
         OledShowString(0, 1, line, 1);
        }
        app_msg = malloc(sizeof(app_msg_t));
        if (app_msg != NULL) {
            app_msg->msg_type = en_msg_report;
            app_msg->msg.report.hongwai = count_speed;
            

           // printf("APP_MSG!!!!!!!!!!!!!!\r\n");
             if (osMessageQueuePut(mid_MsgQueue, &app_msg, 0U, 0U) != 0) {
                free(app_msg);
            }
        
        } 

        sleep(1);
    }
    return 0;
}

//连接wifi
// static int Temperature_SensorTaskEntry(void)
// {

//     while(DHT11_Init())	//DHT11初始化	
// 	{
// 		printf("DHT11 Init Error!!\r\n");
//  		usleep(100000);
// 	}		
//     printf("DHT11 Init Successful!!");

//     int ret;
//     app_msg_t *app_msg;
//     int temp=0;  	    
//     int humi=0;
//     int ledflag =0;

//     while (1) {

//         if( DHT11_Read_Data(&temp,&humi)==0)	//读取温湿度值
//         {   
//           if((temp!= 0)||(humi!=0))
//           {
//             ledflag++;
//             printf("Temperature = %d\r\n",temp);
//             printf("Humidity = %d%%\r\n",humi);
//           }
//         }
//         usleep(1000000);
//         app_msg = malloc(sizeof(app_msg_t));
//         printf("temp:%d hum:%d\r\n", temp, humi);
//         if (app_msg != NULL) {
//             app_msg->msg_type = en_msg_report;
//             app_msg->msg.report.hum = humi;
//             // app_msg->msg.report.lum = temp;
//             app_msg->msg.report.temp = temp;
//             if (osMessageQueuePut(mid_MsgQueue, &app_msg, 0U, 0U) != 0) {
//                 free(app_msg);
//             }
//         }
//         sleep(1);
//     }
//     return 0;
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



static void IotMainTaskEntry(void)
{
    mid_MsgQueue = osMessageQueueNew(MSGQUEUE_COUNT, MSGQUEUE_SIZE, NULL);
    if (mid_MsgQueue == NULL) {
        printf("Failed to create Message Queue!\n");
    }
    OledInit();
    OledFillScreen(0);
    osThreadAttr_t attr;

    attr.name = "CloudMainTaskEntry";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = CLOUD_TASK_STACK_SIZE;
    attr.priority = CLOUD_TASK_PRIO;

    if (osThreadNew((osThreadFunc_t)CloudMainTaskEntry, NULL, &attr) == NULL) {
        printf("Failed to create CloudMainTaskEntry!\n");
    }


    //舵机控制线程
    attr.name = "Servo_Task";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 1024; // 堆栈大小为1024 stack size 1024
    attr.priority = osPriorityNormal;
    if (osThreadNew((osThreadFunc_t)Sg92RTask, NULL, &attr) == NULL) {
        printf("[LedExample] Failed to create LedTask!\n");
    }

    // // 红外线程
    attr.name = "Hongwai_Task";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = SENSOR_TASK_STACK_SIZE*5; // 堆栈大小为1024 stack size 1024
    attr.priority = SENSOR_TASK_PRIO;
    if (osThreadNew((osThreadFunc_t)HONGWAI_SensorTaskEntry, NULL, &attr) == NULL) {
        printf("[LedExample] Failed to create HONGWAI_SensorTaskEntry!\n");
    }

    //水位读取线程
    // attr.name = "Water_SenorInit";
    // attr.attr_bits = 0U;
    // attr.cb_mem = NULL;
    // attr.cb_size = 0U;
    // attr.stack_mem = NULL;
    // attr.stack_size = 10240;
    // attr.priority = 25;
    // if (osThreadNew((osThreadFunc_t)ADCTask, NULL, &attr) == NULL)
    // {
    //     printf("Falied to create WifiAPTask!\r\n");
    // }


//     attr.name = "UDPClientTask";
//     attr.attr_bits = 0U;
//     attr.cb_mem = NULL;
//     attr.cb_size = 0U;
//     attr.stack_mem = NULL;
//     attr.stack_size = 10240;
//     attr.priority = osPriorityNormal;

//     if (osThreadNew((osThreadFunc_t)UDPClientTask, NULL, &attr) == NULL) {
//         printf("[UDPClientDemo] Failed to create UDPClientTask!\n");
//     }
}

APP_FEATURE_INIT(IotMainTaskEntry);