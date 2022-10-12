/**********************************************************************
 * File : app_config.h
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
#ifndef app_config_h
#define app_config_h
#include <stdbool.h>
#include "debug/debugtask.h"

#define Nurmal_PRIORITY					(tskIDLE_PRIORITY+1)

/******************** DEBUG TASK **********************/
#define Debug_PRIORITY		Nurmal_PRIORITY
#define Debug_Port			com1
#define Debug_Task_Stak		2048	/*Stack Size Of Task*/
#define Debug_Buad			115200  /*230400*/
#define Debug_QItem			64      /*64 item in queue*/
#define Debug_QSize			128     /*128 Size Of each Message*/

/**************** OS_Analysers TASK *******************/
#define RTOSAnalysis_PRIORITY			Nurmal_PRIORITY
#define RTOSAnalysis_TASK_Stak			4096	/*Stack Size Of Task*/
#define RTOSAnalysis_zWindow            xWindow11

/******************** PPPOS TASK **********************/
#define PPPOS_PRIORITY			(Nurmal_PRIORITY+1)
#define PPPOS_TASK_Stak			4096
#define PPPOS_zWindow           xWindow21
#define PPPOS_Port			    com3
#define PPPOS_Buad			    921600



#endif /* app_config_h */
