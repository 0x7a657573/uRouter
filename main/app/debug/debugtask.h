/**********************************************************************
 * File : debugtask.h
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
#ifndef debugtask_h
#define debugtask_h
#include <stdint.h>
#include <stddef.h>
#include "uart/uart.h"

#define UNUSED(x) (void)(x)

typedef enum
{
    xWindow11=1,
    xWindow12=5,
    xWindow13=2,
    xWindow14=3,
    xWindow21=6,
    xWindow22=7,
    xWindow23=13,
    xWindow24=9,
    xWindow31=12,
    xWindow32=14,
    xWindow33=15
}xwindow_t;

typedef struct
{
    size_t TaskStack;
    uint32_t TaskPriority;
    size_t ItemLen;
    size_t QueueLen;
    Serialport_t dPort;
    SerialBaud_t dBuad;
}debug_t;



bool  DebugTask_Init(debug_t *dbg);
int  xdebugf (xwindow_t ID, const  char *format, ...);

#endif /* debugtask_h */
