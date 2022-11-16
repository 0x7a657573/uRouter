/**********************************************************************
 * File : uart.c
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
#include <string.h>
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "uart.h"
#include "../driver_config.h"

/*Fifo Size*/
const int uRXFifo[] = {UART1_RX_FIFO_SIZE,UART2_RX_FIFO_SIZE,UART3_RX_FIFO_SIZE};
const int uTXFifo[] = {UART1_TX_FIFO_SIZE,UART2_TX_FIFO_SIZE,UART3_TX_FIFO_SIZE};
/*GPIO Config*/
const int uRXGpio[] = {UART1_RX_PIN,UART2_RX_PIN,UART3_RX_PIN};
const int uTXGpio[] = {UART1_TX_PIN,UART2_TX_PIN,UART3_TX_PIN};


bool xuart_init(Serialport_t port,SerialBaud_t baud)
{
/* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t first_uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    uart_config_t uart_config = {
        .baud_rate = (int)baud,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    int intr_alloc_flags = 0;
#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    if(uart_is_driver_installed((uart_port_t)port))
    {
        ESP_ERROR_CHECK(uart_driver_delete((uart_port_t)port));
    }

    ESP_ERROR_CHECK(uart_driver_install((uart_port_t)port,uRXFifo[port],uTXFifo[port],0,NULL,intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config((uart_port_t)port, &first_uart_config));
    ESP_ERROR_CHECK(uart_set_pin((uart_port_t)port, uTXGpio[port], uRXGpio[port], UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    char tmp[20];
    sprintf(tmp,"AT+IPR=%d\r\n",baud);
    vTaskDelay(500);
    uart_write_bytes((uart_port_t)port,tmp, strlen(tmp));
    vTaskDelay(200);
    ESP_ERROR_CHECK(uart_param_config((uart_port_t)port, &uart_config));
    uart_write_bytes((uart_port_t)port,"AT&W\r\n", 15);
    vTaskDelay(200);
    
    return true;
}


int32_t xuart_writes(Serialport_t port,char *data,size_t len)
{
    return uart_write_bytes((uart_port_t)port, (const char *) data, len);
}

int32_t xuart_reads(Serialport_t port,void *data,size_t max_len,uint32_t timeout_tick)
{
    return uart_read_bytes((uart_port_t)port,data,max_len,timeout_tick);
}

void xuart_flush(Serialport_t port)
{
    uart_flush((uart_port_t)port);
}

void xuart_wait_tx_done(Serialport_t port,uint32_t ticks_to_wait)
{
    uart_wait_tx_done((uart_port_t)port, ticks_to_wait);
}
