/* main.c -- MQTT client example
*
* Copyright (c) 2014-2015, Tuan PM <tuanpm at live dot com>
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* * Neither the name of Redis nor the names of its contributors may be used
* to endorse or promote products derived from this software without
* specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/
#include "ets_sys.h"
#include "osapi.h"
#include "mqtt.h"
#include "wifi.h"
#include "debug.h"
#include "gpio.h"
#include "user_interface.h"
#include "stdout/stdout.h"
#include "sntp.h"
#include "time_utils.h"

MQTT_Client mqttClient;

static char client_ID[25];
static volatile uint32_t chip_ID;

static volatile uint8_t sleep = 0;
static volatile os_timer_t sleep_timer;

void ICACHE_FLASH_ATTR sleep_timer_func(void);
void ICACHE_FLASH_ATTR sntpGotTime(time_t ntp_time);

void sleep_timer_func(void){
	
	INFO("Going for deep sleep. gd night\r\n");
	deep_sleep_set_option(1);
	system_deep_sleep(0);
}
	
void sntpGotTime(time_t ntp_time){
	MQTT_Client* client = (MQTT_Client*)&mqttClient;
	
	char timestamp[100];
	char topic[100];
	sleep = 1;
	
	os_sprintf(topic,"/ESP-PIR/%d", chip_ID);//set topic
	os_sprintf(timestamp,"%s : motion detected at %s\r\n", client_ID, epoch_to_str(ntp_time)); //+8 SG time
	MQTT_Publish(client, topic, timestamp, strlen(timestamp), 1, 0); //publish message
	INFO("got time : %s\r\n",epoch_to_str(ntp_time));	
}

//Call back for WIFI connection
void  wifiConnectCb(uint8_t status)
{
	if(status == STATION_GOT_IP){
		MQTT_Connect(&mqttClient);
	} else {
		MQTT_Disconnect(&mqttClient);
	}
}

//Call back for connect
void mqttConnectedCb(uint32_t *args)
{
	char lwt[100];
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Connected\r\n");
	os_sprintf(lwt,"/ESP-PIR/%d/status", chip_ID); //set LWT
	MQTT_Publish(client, lwt, NULL, 0, 0, 1); //clear LWT	
	sntp_init(sntpGotTime);
}

//Call back for message published
void mqttPublishedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Published\r\n");

	if (sleep){
		//setup timer
		INFO("Wait 1 sec\r\n");
		os_timer_disarm(&sleep_timer);
		os_timer_setfn(&sleep_timer, (os_timer_func_t *)sleep_timer_func, NULL);
		os_timer_arm(&sleep_timer, 1000, 0); //wait 1 sec	  1 shot only
	}
}

void ICACHE_FLASH_ATTR user_init(void)
{
	char lwt[100];
    // Initialize the GPIO subsystem.
    gpio_init();
	
    stdout_init(); //init TXD 9600baud. free up RXD for GPIO

	INFO("init\r\n");
	
	chip_ID = system_get_chip_id();
	os_sprintf(client_ID,"%s-%d", MQTT_CLIENT_ID, chip_ID);
	//SetUp MQTT client
	MQTT_InitConnection(&mqttClient, MQTT_HOST, MQTT_PORT, DEFAULT_SECURITY);
	MQTT_InitClient(&mqttClient, client_ID, MQTT_USER, MQTT_PASS, MQTT_KEEPALIVE, 1);
	os_sprintf(lwt,"/ESP-PIR/%d/status", chip_ID); //set LWT
	MQTT_InitLWT(&mqttClient, lwt, "offline", 0, 1);
	MQTT_OnConnected(&mqttClient, mqttConnectedCb);
//	MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);
	MQTT_OnPublished(&mqttClient, mqttPublishedCb);
	
	os_delay_us(1000000);
	
	//Connect to WIFI
	INFO("connect WIFI\r\n");
	WIFI_Connect(STA_SSID, STA_PASS, wifiConnectCb);

	INFO("\r\nSystem started ...\r\n");
	
}
