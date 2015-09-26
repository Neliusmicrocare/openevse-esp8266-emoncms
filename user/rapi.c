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
#include "rapi.h"


char respBuf[RAPI_BUFLEN];
int rapiTokenCnt;
char *rapiTokens[RAPI_MAX_TOKENS];

char ICACHE_FLASH_ATTR *utoa(unsigned num, char *str, int radix) {
    char temp[17];  //an int can only be 16 bits long
                    //at radix 2 (binary) the string
                    //is at most 16 + 1 null long.
    int temp_loc = 0;
    int digit;
    int str_loc = 0;

    //construct a backward string of the number.
    do {
        digit = (unsigned int)num % radix;
        if (digit < 10) 
            temp[temp_loc++] = digit + '0';
        else
            temp[temp_loc++] = digit - 10 + 'A';
        num = ((unsigned int)num) / radix;
    } while ((unsigned int)num > 0);

    temp_loc--;


    //now reverse the string.
    while ( temp_loc >=0 ) {// while there are still chars
        str[str_loc++] = temp[temp_loc--];    
    }
    str[str_loc] = 0; // add null termination.

    return str;
}

void ICACHE_FLASH_ATTR rapiInit()
{
  *respBuf = 0;
}


void ICACHE_FLASH_ATTR rapiSendCmd(const char *cmdstr)
{
  uart0_sendStr(cmdstr);
  const char *s = cmdstr;
  uint8_t chk = 0;
  while (*s) {
    chk ^= *(s++);
  }
  respBuf[0] = '^';
  utoa(chk,respBuf+1,16);
  strcat(respBuf,"\r");
  uart0_sendStr(respBuf);

  *respBuf = 0;
  rapiTokenCnt = 0;
}


void ICACHE_FLASH_ATTR tokenize()
{
  char *s = respBuf;
  rapiTokenCnt = 0;
  while (*s) {
    rapiTokens[rapiTokenCnt++] = s++;
    if (rapiTokenCnt == RAPI_MAX_TOKENS) break;
    while (*s && (*s != ' ')) s++;
    if (*s == ' ') *(s++) = '\0';
  }
}

/*
 * return values:
 * -1= timeout
 * 0= success
 * 1=$NK
 * 2=invalid RAPI response
*/
int ICACHE_FLASH_ATTR rapiProcessReply()
{
 start:
  rapiTokenCnt = 0;
  *respBuf = 0;

  int bufpos = 0;
  uint32 mss = system_get_time();
  do {
    int rxc = uart0_rx_one_char();
    if (rxc == -1) {
      os_delay_us(10);
    }
    else {
      char c = rxc;

      if (!bufpos && (c != '$')) {
	// wait for start character
	continue;
      }
      else if (c == '\r') {
	respBuf[bufpos] = '\0';
	tokenize();
      }
      else {
	if (c == '$') bufpos = 0;
	respBuf[bufpos++] = c;
	// buffer full and no termination 
	if (bufpos >= (RAPI_BUFLEN-1)) goto start;
      }
    }
  } while (!rapiTokenCnt && ((system_get_time() - mss) < RAPI_TIMEOUT_US));
  
/*  
    dbgprint("\n\rTOKENCNT: ");dbgprintln(rapiTokenCnt);
    for (int i=0;i < rapiTokenCnt;i++) {
      dbgprintln(rapiTokens[i]);
    }
    dbgprintln("");
*/
  

  if (rapiTokenCnt) {
    if (!strcmp(respBuf,"$OK")) {
      return 0;
    }
    else if (!strcmp(respBuf,"$NK")) {
      return 1;
    }
    else if (!strcmp(respBuf,"$WF")) { // async WIFI
      goto start;
    }
    else if (!strcmp(respBuf,"$ST")) { // async EVSE state transition
      goto start;
    }
    else {
      return 2;
    }
  }
  else {
    return -1;
  }
}
