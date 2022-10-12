/**********************************************************************
 * File : debugtask.c
 * Copyright (c) zeus.
 * Created On : Sat Jul 09 2022
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
#include "debugtask.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "string.h"
#include <stdarg.h>

xQueueHandle      xDebugQueue;
SemaphoreHandle_t xDebugMutex;
void DebugMainTask( void *pvParameters );

struct debugparam
{
    Serialport_t port;
    size_t       max_msg_len;
    char        *write_buffer;
};
static struct debugparam dparam = {0};


bool DebugTask_Init(debug_t *dbg)
{
    dparam.port = dbg->dPort;
    dparam.max_msg_len = dbg->ItemLen;
    dparam.write_buffer = malloc(dbg->ItemLen);
    if(!dparam.write_buffer)
        return false;
    
    xDebugQueue = xQueueCreate(dbg->QueueLen,dbg->ItemLen);
	if(xDebugQueue)
		xTaskCreate(DebugMainTask, "Debug", dbg->TaskStack, &dparam, dbg->TaskPriority, NULL );
	xDebugMutex = xSemaphoreCreateMutex();
    xuart_init(dbg->dPort,dbg->dBuad);
    return true;
}


void DebugMainTask( void *pvParameters )
{
    struct debugparam *param = (struct debugparam *)pvParameters;
    char  *buffer = malloc(param->max_msg_len);
    while(!buffer)
        vTaskDelay(2000);
    
    while(1)
    {
        BaseType_t res = xQueueReceive(xDebugQueue,buffer,10);
		if(res==pdPASS)
		{
			xuart_writes(param->port,buffer,strlen(buffer));
		}
    }
}


int  xdebugf(xwindow_t ID, const  char *format, ...)
{
  int Resp  = 0;
  if( xSemaphoreTake( xDebugMutex, ( TickType_t ) 100 ) == pdTRUE )
  {
	va_list     vArgs;
	memset(dparam.write_buffer,0,dparam.max_msg_len);
    /*add Header*/
    dparam.write_buffer[0]= 200;
    dparam.write_buffer[1]= 200+ID;

    va_start(vArgs, format);
    Resp = vsnprintf(&dparam.write_buffer[2],dparam.max_msg_len-4,format,vArgs) + 2;
    
    dparam.write_buffer[Resp] = 250;
    dparam.write_buffer[Resp+1] = 0;
    va_end(vArgs);
    if(xQueueSend(xDebugQueue,dparam.write_buffer,1)!= pdPASS)
    {
    
    }
    xSemaphoreGive( xDebugMutex );
  }

  return Resp;
}