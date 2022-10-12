/**********************************************************************
 * File : uart.h
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
#ifndef uart_h
#define uart_h
#include <stdint.h>
#include <stdbool.h>


typedef enum
{
    b110=110,
    b300=300,
    b600=600,
    b1200=1200,
    b2400=2400, 
    b4800=4800, 
    b9600=9600, 
    b14400=14400, 
    b19200=19200, 
    b38400=38400, 
    b57600=57600, 
    b115200=115200,
    b230400=230400,
    b460800=460800,
    b921600=921600
}SerialBaud_t;

typedef enum
{
    com_null=-1,
    com1=0,
    com2,
    com3,
}Serialport_t;


bool xuart_init(Serialport_t port,SerialBaud_t baud);
int32_t xuart_writes(Serialport_t port,char *data,size_t len);
int32_t xuart_reads(Serialport_t port,void *data,size_t max_len,uint32_t timeout_tick);
void xuart_flush(Serialport_t port);
void xuart_wait_tx_done(Serialport_t port,uint32_t ticks_to_wait);
#endif /* uart_h */
