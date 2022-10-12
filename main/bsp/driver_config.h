/**********************************************************************
 * File : driver_config.h
 * Copyright (c) Author.
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
#ifndef driver_config_h
#define driver_config_h

/******************************** Uart Config *****************************/
#define UART1_RX_FIFO_SIZE      512
#define UART1_TX_FIFO_SIZE      512
#define UART1_RX_PIN            GPIO_NUM_3
#define UART1_TX_PIN            GPIO_NUM_1

#define UART2_RX_FIFO_SIZE      512
#define UART2_TX_FIFO_SIZE      512
#define UART2_RX_PIN            GPIO_NUM_9
#define UART2_TX_PIN            GPIO_NUM_10

#define UART3_RX_FIFO_SIZE      512
#define UART3_TX_FIFO_SIZE      512
#define UART3_RX_PIN            GPIO_NUM_16
#define UART3_TX_PIN            GPIO_NUM_4 //GPIO_NUM_17


#endif /* driver_config.h */
