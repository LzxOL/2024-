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
#include "dht11.h"
#include "iot_gpio_ex.h"
#include "iot_gpio.h"

u8 ledflag=0;
u8 temp=0;  	    
u8 humi=0;



// static void DHT11ExampleEntry(void)
// {
//     osThreadAttr_t attr;

//     attr.name = "DHT11_Task";
//     attr.attr_bits = 0U;
//     attr.cb_mem = NULL;
//     attr.cb_size = 0U;
//     attr.stack_mem = NULL;
//     attr.stack_size = 1024 ;
//     attr.priority = 25;

//     if (osThreadNew((osThreadFunc_t)DHT11_Task, NULL, &attr) == NULL)
//     {
//         printf("Falied to create DHT11_Task!\n");
//     }
// }


// APP_FEATURE_INIT(DHT11ExampleEntry);
// #define	DHT11_DQ_IN  	Gpio_Input_Action(); //读取GPIO输入的状态
IotGpioValue DHT11_DQ_IN;
IotGpioValue levelold;//IO输出状况
#define DHT11_GPIO  IOT_IO_NAME_GPIO_0


u8 GPIOGETINPUT(IotIoName id,IotGpioValue *val)
{
    IoTGpioGetInputVal(id,val);
    return *val;
}

/****************************************
设置端口为输出
*****************************************/
void DHT11_IO_OUT(void)
{
    //设置GPIO_9为输出模式
    IoTGpioSetDir(DHT11_GPIO, IOT_GPIO_DIR_OUT);
}

/****************************************
设置端口为输入
*****************************************/
void DHT11_IO_IN(void)
{
    IoTGpioSetDir(DHT11_GPIO, IOT_GPIO_DIR_IN);
	IoSetPull(DHT11_GPIO,IOT_IO_PULL_NONE);
}


//复位DHT11
void DHT11_Rst(void)	   
{                
	DHT11_IO_OUT(); 	//SET OUTPUT
   DHT11_DQ_OUT_Low; 	//拉低DQ
    // GpioGetOutputVal(DHT11_GPIO,&levelold);
    // printf("out:%d\r\n",levelold);
    hi_udelay(20000);//拉低至少18ms
   DHT11_DQ_OUT_High; 	//DQ=1 
	hi_udelay(35);     	//主机拉高20~40us
  //  GpioGetOutputVal(DHT11_GPIO,&levelold);
  //   printf("out:%d\r\n",levelold);
  //    printf("DHT11 Rest Successful\r\n");
}
//等待DHT11的回应
//返回1:未检测到DHT11的存在
//返回0:存在
u8 DHT11_Check(void) 	   
{   
	u8 retry=0;
	  DHT11_IO_IN();//SET INPUT	 
    while (GPIOGETINPUT(DHT11_GPIO,&DHT11_DQ_IN)&&retry<100)//DHT11会拉低40~80us
	{
		retry++;
		hi_udelay(1);
	};	 
	if(retry>=100)return 1;
	else retry=0;
    
    while ((!GPIOGETINPUT(DHT11_GPIO,&DHT11_DQ_IN))&&retry<100)//DHT11拉低后会再次拉高40~80us
	{
		retry++;
		hi_udelay(1);
	};
	if(retry>=100)return 1;	   
	return 0;
}
//从DHT11读取一个位
//返回值：1/0
u8 DHT11_Read_Bit(void) 			 
{
 	u8 retry=0;
  while(GPIOGETINPUT(DHT11_GPIO,&DHT11_DQ_IN)&&retry<100){//等待变为低电平
        retry++;
        hi_udelay(1);
    }
    retry=0;
    while((!GPIOGETINPUT(DHT11_GPIO,&DHT11_DQ_IN))&&retry<100){//等待变高电平
        retry++;
        hi_udelay(1);
    }
    hi_udelay(40);//等待40us	//用于判断高低电平，即数据1或0
    if(GPIOGETINPUT(DHT11_GPIO,&DHT11_DQ_IN))return 1; else return 0;
}
//从DHT11读取一个字节
//返回值：读到的数据
u8 DHT11_Read_Byte(void)    
{        
    u8 i,dat;
    dat=0;
	for (i=0;i<8;i++) 
	{
   		dat<<=1; 
	    dat|=DHT11_Read_Bit();
    }						    
    return dat;
}
//从DHT11读取一次数据
//temp:温度值(范围:0~50°)
//humi:湿度值(范围:20%~90%)
//返回值：0,正常;1,读取失败
u8 DHT11_Read_Data(u8 *temp,u8 *humi)    
{        
 	u8 buf[5]={ 0 };
	u8 i;
	DHT11_Rst();
	if(DHT11_Check()==0)
	{
		for(i=0;i<5;i++)//读取40位数据
		{
			buf[i]=DHT11_Read_Byte();
		}
		if((buf[0]+buf[1]+buf[2]+buf[3])==buf[4])//数据校验
		{
			*humi=buf[0];
			*temp=buf[2];
			
		}
	}else return 1;
	return 0;	    
}
//初始化DHT11的IO口 DQ 同时检测DHT11的存在
//返回1:不存在
//返回0:存在    	 
u8 DHT11_Init(void)
{	 
	//初始化GPIO
    // IoTGpioInit(IOT_IO_NAME_GPIO_2);
	IoTGpioInit(IOT_IO_NAME_GPIO_0);
    //设置GPIO_2的复用功能为普通GPIO
    // IoSetFunc(IOT_IO_NAME_GPIO_2, IOT_IO_FUNC_GPIO_2_GPIO);
    // //设置GPIO_2为输出模式
    // IoTGpioSetDir(IOT_IO_NAME_GPIO_2, IOT_GPIO_DIR_OUT);
	// //设置GPIO_2输出高电平点亮LED灯
    // IoTGpioSetOutputVal(IOT_IO_NAME_GPIO_2, 1);


    //设置GPIO_9的复用功能为普通GPIO 
	IoSetFunc(IOT_IO_NAME_GPIO_0, IOT_IO_FUNC_GPIO_0_GPIO);
    //设置GPIO_9为输出模式
    IoTGpioSetDir(IOT_IO_NAME_GPIO_0, IOT_GPIO_DIR_OUT);
		 //设置GPIO_9输出高电平
    IoTGpioSetOutputVal(IOT_IO_NAME_GPIO_0, 1);		    
	DHT11_Rst();  //复位DHT11
	return DHT11_Check();//等待DHT11的回应
} 


void DHT11_Task(void)
{  

    while(DHT11_Init())	//DHT11初始化	
	{
		printf("DHT11 Init Error!!\r\n");
 		usleep(100000);
	}		
    printf("DHT11 Init Successful!!");
    while (1)
    {
       if( DHT11_Read_Data(&temp,&humi)==0)	//读取温湿度值
        {   
          if((temp!= 0)||(humi!=0))
          {
            ledflag++;
            printf("Temperature = %d\r\n",temp);
            printf("Humidity = %d%%\r\n",humi);
          }
        }
        //延时100ms
    //    IoTGpioSetOutputVal(WIFI_IOT_GPIO_IDX_2, ledflag%2);
        usleep(1000000);
    }
}
