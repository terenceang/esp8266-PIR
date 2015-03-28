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
#include "config.h"
#include "debug.h"
#include "gpio.h"
#include "user_interface.h"
#include "stdout/stdout.h"
#include "sntp.h"
#include "time_utils.h"

MQTT_Client mqttClient;

static volatile os_timer_t sntp_retry_timer;
static volatile int sleep = 0;


//Call back for WIFI connection
void wifiConnectCb(uint8_t status)
{
	if(status == STATION_GOT_IP){

		os_delay_us(1000000); //1 sec
				sntp_init(0);
		os_delay_us(1000000); //1 sec
		
		MQTT_Connect(&mqttClient);
	} else {
		MQTT_Disconnect(&mqttClient);
	}
}

//Call back for connect
void mqttConnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Connected\r\n");
	MQTT_Publish(client, "/ESP-PIR/status", "", 0, 0, 1); //clear LWT	
	os_timer_arm(&sntp_retry_timer, 1000, 1);
	INFO("Arm timer\r\n");
}

//Call back for disconnect
void mqttDisconnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Disconnected\r\n");
}

//Call back for message published
void mqttPublishedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Published\r\n");
	
	if (sleep){
		
	  INFO("Going for deep sleep. gd night\r\n");
	  
	  deep_sleep_set_option(1);
	  system_deep_sleep(0);
	}
}

void sntp_retry_timer_func(void *arg)
{
		MQTT_Client* client = (MQTT_Client*)&mqttClient;
		INFO("retry\r\n");
		
		if (sntp_time > 0)
		{
		  char timestamp[100];
		  os_sprintf(timestamp,"motion detected at %s\r\n",epoch_to_str(sntp_time + 28800 + 0)); //+8 SG time
		  MQTT_Publish(client, "/ESP-PIR/status",timestamp, strlen(timestamp), 1, 0); //send hello
	      //Disarm timer
		  INFO("Disarrm timer\r\n");
    	  os_timer_disarm(&sntp_retry_timer);
		  
		  INFO("got time : %s\r\n",epoch_to_str(sntp_time));
		  
		  sleep = 1;		  
		}
}


void user_init(void)
{
    // Initialize the GPIO subsystem.
    gpio_init();
	
    stdout_init(); //init TXD 9600baud. free up RXD for GPIO

	INFO("init\r\n");
	os_delay_us(1000000);

	CFG_Load(); //Load CFG SSID not loaded correctly on ESP8266-01
	
    //Setup timer
	INFO("set up timer\r\n");
	os_timer_disarm(&sntp_retry_timer);
    os_timer_setfn(&sntp_retry_timer, (os_timer_func_t *)sntp_retry_timer_func, NULL);
	
	//SetUp MQTT client
	MQTT_InitConnection(&mqttClient, sysCfg.mqtt_host, sysCfg.mqtt_port, sysCfg.security);
	MQTT_InitClient(&mqttClient, sysCfg.device_id, sysCfg.mqtt_user, sysCfg.mqtt_pass, sysCfg.mqtt_keepalive, 1);
	MQTT_InitLWT(&mqttClient, "/ESP-PIR/status", "offline", 0, 1);
	MQTT_OnConnected(&mqttClient, mqttConnectedCb);
	MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);
	MQTT_OnPublished(&mqttClient, mqttPublishedCb);
	
	//Connect to WIFI
	INFO("connect WIFI\r\n");
	WIFI_Connect(sysCfg.sta_ssid, sysCfg.sta_pwd, wifiConnectCb);

	INFO("\r\nSystem started ...\r\n");
	
}
