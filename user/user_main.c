/*
 * Copyright (c) 2015 Sam C. Lin
 *
 * This file is part of Open EVSE.
 * Open EVSE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * Open EVSE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with Open EVSE; see the file COPYING.  If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


#include "ets_sys.h"
#include "driver/uart.h"
#include "osapi.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "rapi.h"
#include "httpclient.h"

//#define dbghttp
//#define dbgfakedata

char ssid[33] = SSID;
char pass[65] = PASS;
char privateKey[33] = PRIVATEKEY;
char node[5] = NODE;
const char *url = "http://data.openevse.com/emoncms/input/post.json?";
const char* inputID_AMP   = "OpenEVSE_AMP:";
const char* inputID_VOLT   = "OpenEVSE_VOLT:";
const char* inputID_TEMP1   = "OpenEVSE_TEMP1:";
const char* inputID_TEMP2   = "OpenEVSE_TEMP2:";
const char* inputID_TEMP3   = "OpenEVSE_TEMP3:";
const char* inputID_PILOT   = "OpenEVSE_PILOT:";


#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];
static void user_procTask(os_event_t *events);

static volatile os_timer_t some_timer;


int amp = 0;
int32_t volt = 0;
int temp1 = 0;
int temp2 = 0;
int temp3 = 0;
int pilot = 0;

char* ICACHE_FLASH_ATTR ltoa( long value, char *string, int radix )
{
  char tmp[33];
  char *tp = tmp;
  long i;
  unsigned long v;
  int sign;
  char *sp;

  if ( string == NULL )
  {
    return 0 ;
  }

  if (radix > 36 || radix <= 1)
  {
    return 0 ;
  }

  sign = (radix == 10 && value < 0);
  if (sign)
  {
    v = -value;
  }
  else
  {
    v = (unsigned long)value;
  }

  while (v || tp == tmp)
  {
    i = v % radix;
    v = v / radix;
    if (i < 10)
      *tp++ = i+'0';
    else
      *tp++ = i + 'a' - 10;
  }

  sp = string;

  if (sign)
    *sp++ = '-';
  while (tp > tmp)
    *sp++ = *--tp;
  *sp = 0;

  return string;
}

char* ICACHE_FLASH_ATTR itoa( int value, char *string, int radix )
{
  return ltoa( value, string, radix ) ;
}


// convert decimal string to int32_t
int32 ICACHE_FLASH_ATTR dtoi32(const char *s)
{
  int32 i = 0;
  int8 sign;
  if (*s == '-') {
    sign = -1;
    s++;
  }
  else {
    sign = 0;
  }
  while (*s) {
    i *= 10;
    i += *(s++) - '0';
  }

  if (sign < 0) i = -i;
  return i;
}


void httpCallback(char * response, int http_status, char * full_response)
{
#ifdef dbghttp
    os_printf("http_status=%d\n", http_status);
    if (http_status != HTTP_STATUS_GENERIC_ERROR) {
        os_printf("strlen(full_response)=%d\n", strlen(full_response));
        os_printf("response=%s<EOF>\n", response);
    }
#endif
}

int tCnt;

void ICACHE_FLASH_ATTR some_timerfunc(void *arg)
{
  tCnt++;
  if ((wifi_station_get_connect_status() != STATION_GOT_IP) || (tCnt == 50)) {
    tCnt = -1;
  }
  else {
    if (tCnt == 0) {
      rapiSendCmd("$GE");
    }
    else if (tCnt == 1) {
      if (rapiTokenCnt == 3) {
	pilot = atoi(rapiTokens[1]);
      }
    }
    else if (tCnt == 2) {
      rapiSendCmd("$GG");
    }
    else if (tCnt == 3) {
      if (rapiTokenCnt == 3) {
	amp = atoi(rapiTokens[1]);
	volt = dtoi32(rapiTokens[2]);
	if (volt < 0) volt = 0;
      }
    }
    else if (tCnt == 4) {
      rapiSendCmd("$GP");
    }
    else if (tCnt == 5) {
      if (rapiTokenCnt == 4) {
	temp1 = atoi(rapiTokens[1]);
	temp2 = atoi(rapiTokens[2]);
	temp3 = atoi(rapiTokens[3]);
      }
    }
    else if (tCnt == 6) {
      char req[300],s[80];

#ifdef dbgfakedata
      volt = 238945;
      temp1 = 136;
      temp2 = 235;
      temp3 = 335;
      amp = 16023;
      pilot = 16;
#endif

      os_strcpy(req,url);
      os_strcat(req,"node=");
      os_strcat(req,node);
      os_strcat(req,"&apikey=");
      os_strcat(req,privateKey);
      os_strcat(req,"&json={");
      
      os_strcat(req,inputID_AMP);
      itoa(amp,s,10);
      os_strcat(req,s);
      
      os_strcat(req,",");
      os_strcat(req,inputID_PILOT);
      itoa(pilot,s,10);
      os_strcat(req,s);
      
      if (volt >= 0) {
	os_strcat(req,",");
	os_strcat(req,inputID_VOLT);
	ltoa(volt,s,10);
	os_strcat(req,s);
      }
      if (temp1 > 0) {
	os_strcat(req,",");
	os_strcat(req,inputID_TEMP1);
	itoa(temp1,s,10);
	os_strcat(req,s);
      }
      if (temp2 > 0) {
	os_strcat(req,",");
	os_strcat(req,inputID_TEMP2);
	itoa(temp2,s,10);
	os_strcat(req,s);
      }
      if (temp3 > 0) {
	os_strcat(req,",");
	os_strcat(req,inputID_TEMP3);
	itoa(temp3,s,10);
	os_strcat(req,s);
      }
      os_strcat(req,"}");
      
#ifdef dbghttp
    os_printf("\n%s\n",req);
#endif
      http_get(req,"",httpCallback);
    }
  }
}

//Do nothing function
static void ICACHE_FLASH_ATTR
user_procTask(os_event_t *events)
{
  //    os_delay_us(10);
}

//Init function 
void ICACHE_FLASH_ATTR
user_init()
{
  tCnt = -1;

  struct station_config stationConf;
  
  //Set station mode
  wifi_set_opmode(STATION_MODE);

    
  //Set ap settings
  stationConf.bssid_set = 0; //need not check MAC address of AP
  os_memcpy(&stationConf.ssid, ssid, 32);
  os_memcpy(&stationConf.password, pass, 64);
  wifi_station_set_config(&stationConf);

  rapiInit();

  uart_init(BIT_RATE_115200, BIT_RATE_115200);

  //Disarm timer
  os_timer_disarm(&some_timer);
  
  //Setup timer
  os_timer_setfn(&some_timer, (os_timer_func_t *)some_timerfunc, NULL);
  
  //Arm the timer
  //&some_timer is the pointer
  //1000 is the fire time in ms
  //0 for once and 1 for repeating
  os_timer_arm(&some_timer, 500, 1);
  
  //Start os task
  system_os_task(user_procTask, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);
}
