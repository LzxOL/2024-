
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
#include "hongwai.h"
#include "E53_IA1.h"
#include "oc_mqtt.h"
#include "dht11.h"
#include "servo.h"
#include "ASR.h"

#define MSGQUEUE_COUNT 16 // number of Message Queue Objects
#define MSGQUEUE_SIZE 10
#define CLOUD_TASK_STACK_SIZE (1024 * 10)
#define CLOUD_TASK_PRIO 24
#define SENSOR_TASK_STACK_SIZE (1024 * 2)
#define SENSOR_TASK_PRIO 25
#define TASK_DELAY_3S 3
#define HONGAWI_TASK_PRIO 26
#define HONGAWI_TASK_STACK_SIZE (1024 * 2)


extern int count_hongwai;
typedef struct { // object data type
    char *Buf;
    uint8_t Idx;
} MSGQUEUE_OBJ_t;

MSGQUEUE_OBJ_t msg;
osMessageQueueId_t mid_MsgQueue; // message queue id


// https://iot-tool.obs-website.cn-north-4.myhuaweicloud.com/

#define CLIENT_ID "6635a27571d845632a08469b_20240509_0_1_2024050912"
#define USERNAME "6635a27571d845632a08469b_20240509"
#define PASSWORD "3b0156036adc4ed054245d758b3727a6f2726a5ff2fb447fa6bb1ffa1e02c5a4"


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
    int hongwai;
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
    service.service_property = &hongwai_oc;
    service.nxt = NULL;

    hongwai_oc.key = "Hongwai_Count";//滴液检测次数
    hongwai_oc.value = &report->hongwai;
    hongwai_oc.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    hongwai_oc.nxt = NULL; //下一个变量为空

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
            LightStatusSet(ON);
            printf("Light On!");
        } else {
            g_app_cb.led = 0;
            LightStatusSet(OFF);
            printf("Light Off!");
        }
        cmdret = 0;
    } else if (strcmp(cJSON_GetStringValue(obj_cmdname), "Agriculture_Control_Motor") == 0) {
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
            g_app_cb.motor = 1;
            MotorStatusSet(ON);
            printf("Motor On!");
        } else {
            g_app_cb.motor = 0;
            MotorStatusSet(OFF);
            printf("Motor Off!");
        }
        cmdret = 0;
    }

    cJSON_Delete(obj_root);
}

//连接wifi
static int CloudMainTaskEntry(void)
{
    app_msg_t *app_msg;

    uint32_t ret = WifiConnect("xiaomi 12spro", "123456789");

    device_info_init(CLIENT_ID, USERNAME, PASSWORD);
    oc_mqtt_init();
    oc_set_cmd_rsp_cb(MsgRcvCallback);

    while (1) {
        app_msg = NULL;
        (void)osMessageQueueGet(mid_MsgQueue, (void **)&app_msg, NULL, 0U);
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
    int ret;
    app_msg_t *app_msg;
    E53IA1Data data;
    ret = E53IA1Init();
    if (ret != 0) {
        printf("E53_IA1 Init failed!\r\n");
        return -1;
    }
    while (1) {
        ret = E53IA1ReadData(&data);
        if (ret != 0) {
            printf("E53_IA1 Read Data failed!\r\n");
            return;
        }
        app_msg = malloc(sizeof(app_msg_t));
        printf("SENSOR:lum:%.2f temp:%.2f hum:%.2f\r\n", data.Lux, data.Temperature, data.Humidity);
        if (app_msg != NULL) {
            app_msg->msg_type = en_msg_report;
            app_msg->msg.report.hum = (int)data.Humidity;
            app_msg->msg.report.lum = (int)data.Lux;
            app_msg->msg.report.temp = (int)data.Temperature;
            if (osMessageQueuePut(mid_MsgQueue, &app_msg, 0U, 0U) != 0) {
                free(app_msg);
            }
        }
        sleep(TASK_DELAY_3S);
    }
    return 0;
}



static int HONGWAI_SensorTaskEntry(void)
{   
    app_msg_t *app_msg;
    while (1) {
        Hongwai_IO_Init();
        app_msg = malloc(sizeof(app_msg_t));
        printf("Hongwai Count:%d\r\n",count_hongwai);
        if (app_msg != NULL) {
            app_msg->msg_type = en_msg_report;
            app_msg->msg.report.hongwai = count_hongwai;
            printf("APP_MSG!!!!!!!!!!!!!!\r\n");
            printf("APP_MSG!!!!!!!!!!!!!!\r\n");
            printf("APP_MSG!!!!!!!!!!!!!!\r\n");
            if (osMessageQueuePut(mid_MsgQueue, &app_msg, 0U, 0U) != 0) {
                free(app_msg);
            }
        }
        sleep(TASK_DELAY_3S);
    }
    return 0;
}


static void IotMainTaskEntry(void)
{
    mid_MsgQueue = osMessageQueueNew(MSGQUEUE_COUNT, MSGQUEUE_SIZE, NULL);
    if (mid_MsgQueue == NULL) {
        printf("Failed to create Message Queue!\n");
    }

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

    //红外线程
    attr.name = "Hongwai_Task";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = SENSOR_TASK_STACK_SIZE; // 堆栈大小为1024 stack size 1024
    attr.priority = SENSOR_TASK_PRIO;
    if (osThreadNew((osThreadFunc_t)HONGWAI_SensorTaskEntry, NULL, &attr) == NULL) {
        printf("[LedExample] Failed to create HONGWAI_SensorTaskEntry!\n");
    }

    // //舵机控制线程
    // attr.name = "Servo_Task";
    // attr.attr_bits = 0U;
    // attr.cb_mem = NULL;
    // attr.cb_size = 0U;
    // attr.stack_mem = NULL;
    // attr.stack_size = 1024; // 堆栈大小为1024 stack size 1024
    // attr.priority = osPriorityNormal;
    // if (osThreadNew((osThreadFunc_t)Sg92RTask, NULL, &attr) == NULL) {
    //     printf("[LedExample] Failed to create LedTask!\n");
    // }

    // //ASR语音助手
    attr.name = "ASR_Init";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 10240;
    attr.priority = 25;
    if (osThreadNew((osThreadFunc_t)ASR_Init, NULL, &attr) == NULL)
    {
        printf("Falied to create WifiAPTask!\r\n");
    }

    //温湿度线程
    attr.name = "DHT11_Task";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 1024 ;
    attr.priority = 25;

    if (osThreadNew((osThreadFunc_t)DHT11_Task, NULL, &attr) == NULL)
    {
        printf("Falied to create DHT11_Task!\n");
    }
}

APP_FEATURE_INIT(IotMainTaskEntry);