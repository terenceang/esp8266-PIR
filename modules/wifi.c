#include "wifi.h"
#include "user_interface.h"
#include "osapi.h"
#include "espconn.h"
#include "os_type.h"
#include "mem.h"
#include "mqtt_msg.h"
#include "debug.h"
#include "user_config.h"

static os_timer_t wifi_timer;
static os_timer_t scan_timer;
WifiCallback wifiCb = NULL;
static uint8_t wifiStatus = STATION_IDLE, last_wifiStatus = NULL;

uint8_t ssid_scan_done, ssid_found;
static volatile uint8_t ap_retry = AP_RETRY;;

static void ICACHE_FLASH_ATTR wifi_check_ip(void *arg)
{
	struct ip_info ipConfig;

	if (ap_retry){
		
		os_timer_disarm(&wifi_timer);
		
		wifi_get_ip_info(STATION_IF, &ipConfig);
		wifiStatus = wifi_station_get_connect_status();
	
		
		if (wifiStatus == STATION_GOT_IP && ipConfig.ip.addr != 0)
		{
		    char server_ip_string[25];
    
			os_sprintf(server_ip_string,"%d.%d.%d.%d\r\n",
		    *((uint8 *)&(ipConfig.ip.addr)), *((uint8 *)&(ipConfig.ip.addr) + 1),
		    *((uint8 *)&(ipConfig.ip.addr) + 2), *((uint8 *)&(ipConfig.ip.addr) + 3));
			
			wifi_station_set_auto_connect(TRUE);
			
			os_timer_setfn(&wifi_timer, (os_timer_func_t *)wifi_check_ip, NULL);
			os_timer_arm(&wifi_timer, 2000, 0);
			wifiCb(wifiStatus);

		}
		else
		{
			if(wifi_station_get_connect_status() == STATION_CONNECTING)
			{
				if (wifiStatus == last_wifiStatus) INFO("STATION_CONNECTING\r\n");
			}	
			else if(wifi_station_get_connect_status() == STATION_WRONG_PASSWORD)
			{
				INFO("STATION_WRONG_PASSWORD - retry : %d\r\n", ap_retry);
				ap_retry--;
				wifi_station_connect();
			}
			else if(wifi_station_get_connect_status() == STATION_NO_AP_FOUND)
			{
				INFO("STATION_NO_AP_FOUND - retry : %d\r\n", ap_retry);
				ap_retry--;
				wifi_station_connect();
			}
			else if(wifi_station_get_connect_status() == STATION_CONNECT_FAIL)
			{
				INFO("STATION_CONNECT_FAIL - retry : %d\r\n", ap_retry);
				ap_retry--;
				wifi_station_connect();
			}
			else
			{
				if (wifiStatus == last_wifiStatus) INFO("STATION_IDLE : %d\r\n", wifiStatus);
			}

			os_timer_setfn(&wifi_timer, (os_timer_func_t *)wifi_check_ip, NULL);
			os_timer_arm(&wifi_timer, 500, 0);
		}
	}
	else
	{
		INFO("unable to connect to AP.\r\n");
		INFO("Going for deep sleep. gd night\r\n");
		deep_sleep_set_option(1);
		system_deep_sleep(0);
	}
}

void ICACHE_FLASH_ATTR WIFI_Connect(uint8_t* ssid, uint8_t* pass, WifiCallback cb)
{
	struct station_config stationConf;

	INFO("Connect to AP\r\n");
	wifi_set_opmode(STATION_MODE);
	wifi_station_set_auto_connect(FALSE);
	wifiCb = cb;

	os_memset(&stationConf, 0, sizeof(struct station_config));

	os_sprintf(stationConf.ssid, "%s", ssid);
	os_sprintf(stationConf.password, "%s", pass);

	wifi_station_set_config(&stationConf);

	os_timer_disarm(&wifi_timer);
	os_timer_setfn(&wifi_timer, (os_timer_func_t *)wifi_check_ip, NULL);
	os_timer_arm(&wifi_timer, 1000, 0);

	wifi_station_connect();
}


void ICACHE_FLASH_ATTR scan_done_callback(void *arg, STATUS status)
{
    struct bss_info *bss = arg;
	
	ssid_found = 0;
    bss = STAILQ_NEXT(bss, next); // ignore first

    while (bss)
    {
		if(strcmp(bss->ssid,STA_SSID)==0) ssid_found++;
        INFO("%d : SSID:%s CH:%d RSSI:%d Authmode:%d\n", ssid_found, bss->ssid, bss->channel, bss->rssi, bss->authmode);
        bss = STAILQ_NEXT(bss, next);
    }
	
	ssid_scan_done = 1;
}


void ICACHE_FLASH_ATTR scan_wifi(){
		//Connect to WIFI
		INFO("scan SSID\r\n");
	
		struct scan_config config;
	
		os_memset(&config, 0, sizeof(struct scan_config));
	
		char *ap_ssid = STA_SSID;
		config.ssid = ap_ssid;
	
		//WIFI_Connect(STA_SSID, STA_PASS, wifiConnectCb);
		wifi_set_opmode(STATION_MODE);
		wifi_station_set_auto_connect(FALSE);

		wifi_station_scan(&config, scan_done_callback);
}