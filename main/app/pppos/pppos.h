/**********************************************************************
 * File : pppos.h
 * Copyright (c) Author.
 * Created On : Wed Jul 13 2022
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/
#ifndef pppos_h
#define pppos_h
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "uart/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "netif/ppp/pppapi.h"

typedef  enum
{
    GSM_STATE_DISCONNECTED=0,
    GSM_STATE_CONNECTED=1,
    GSM_STATE_IDLE=89,
    GSM_STATE_FIRSTINIT=98
}pppstatus_t;

typedef struct 
{
    Serialport_t dPort;
    SerialBaud_t dBuad;
    char         ppp_user[32];
    char         ppp_pass[32];
    char         APN_name[32];
}pppcfg_t;

typedef struct
{
    size_t   TaskStack;
    uint32_t TaskPriority;
    uint32_t logWin;
    pppcfg_t pppos;   
}ppposConfig_t;

typedef struct
{
    ppp_pcb        *ppp;
    struct netif    ppp_netif;
    QueueHandle_t  mutex;
    pppcfg_t       cfg;
    pppstatus_t    status;
    uint32_t       tx_count;
    uint32_t       rx_count;
    uint32_t       connection_time;
    char           *RcvBuffer;
}pppos_t;


typedef struct
{
    pppstatus_t status;
    uint32_t connection_time;
    uint32_t rx_kb;
    uint32_t tx_kb;
}ppp_statistics;


pppos_t  *pppos_Init(ppposConfig_t *cfg);
ppp_statistics *pppos_statistics(pppos_t *ppp,ppp_statistics *stat);

#endif /* pppos_h */
