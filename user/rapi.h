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

#pragma once

#define RAPI_TIMEOUT_US 250000
#define RAPI_BUFLEN 32
#define RAPI_MAX_TOKENS 5

extern char *rapiTokens[RAPI_MAX_TOKENS];
int rapiTokenCnt;

void ICACHE_FLASH_ATTR rapiInit();
int ICACHE_FLASH_ATTR rapiSendCmd(const char *cmdstr);
