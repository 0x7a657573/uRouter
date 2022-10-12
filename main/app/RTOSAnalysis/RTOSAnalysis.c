/**********************************************************************
 * File : RTOSAnalysis.c
 * Copyright (c) zeus.
 * Created On : Mon Jul 11 2022
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
#include "RTOSAnalysis.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "debug/debugtask.h"

static uint32_t logWindow = xWindow11;
#define debug(...)      xdebugf(logWindow,__VA_ARGS__)
#define Errdebug(...)	xdebugf(Window24,__VA_ARGS__)

void RTOSAnalysisTask(void *Ptr);
void RTOSAnalysis_Init(uint32_t TaskStack,uint32_t TaskPriority,uint32_t logWin)
{
    logWindow = logWin;
	xTaskCreate(RTOSAnalysisTask, "OS Analysis", TaskStack, NULL, TaskPriority, NULL );
}

void RTOSAnalysisTask(void *Ptr)
{
	uint32_t ulTotalTime;
    while(1)
    {
        UBaseType_t uxArraySize = uxTaskGetNumberOfTasks(); /*Get Number Of Current Task*/
        debug("Total Task: %i",uxArraySize);

        debug("Name         minStack        CPU(%%)");
        TaskStatus_t *pxTaskStatusArray = pvPortMalloc( uxArraySize * sizeof( TaskStatus_t ) );
		if(pxTaskStatusArray)
		{
			uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, &ulTotalTime );

			for(UBaseType_t x = 0; x <= uxArraySize; x++ )
			{
				for(UBaseType_t y = 0; y < uxArraySize; y++)
				{
					if(pxTaskStatusArray[ y ].xTaskNumber == x)
					{
                        TaskStatus_t *TaskStatus = &pxTaskStatusArray[y];
						float CpuLoad = ((float)TaskStatus->ulRunTimeCounter/configNUM_CORES) / ((float)ulTotalTime) * 100.f;
						debug("%s\t%i\t%0.2f%%",TaskStatus->pcTaskName,TaskStatus->usStackHighWaterMark,CpuLoad);
						break;
					}
				}
			}
			vPortFree( pxTaskStatusArray );
		}
        debug("");
        vTaskDelay(1000);    
    }
}