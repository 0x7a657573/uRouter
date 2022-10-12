/**********************************************************************
 * File : pppos.c
 * Copyright (c) zeus.
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
#include "pppos.h"

#include "debug/debugtask.h"
#include <string.h>
#include "GsmCmd.h"

#include "tcpip_adapter.h"
#include "netif/ppp/pppos.h"
#include "netif/ppp/ppp.h"

#define RECIVE_BUF_SIZE    4096
#define PPPOSMUTEX_TIMEOUT (1000 / portTICK_RATE_MS)


static uint32_t logWindow = xWindow11;
#define AT_debug(...)      xdebugf(xWindow22,__VA_ARGS__)
#define debug(...)      xdebugf(logWindow,__VA_ARGS__)
#define Errdebug(...)	xdebugf(Window24,__VA_ARGS__)

void ppposMainTask( void *pvParameters );
int gsm_RunCmdWaitResponse(Serialport_t port,char * cmd, char *resp, char * resp1, int cmdSize, int timeout, char **response, int size);
void force_disconnect(Serialport_t port,bool rfOff);
static void enableAllInitCmd();
static bool GSM_initialize(Serialport_t port,GSM_Cmd **GSM_cmd,size_t cmd_len);
uint32_t GSM_InitializeBuadRate(Serialport_t port);

static u32_t ppp_output_callback(ppp_pcb *pcb, u8_t *data, u32_t len, void *ctx)
{
    pppos_t *pppos = (pppos_t *)ctx;
    Serialport_t port =  pppos->cfg.dPort;
	uint32_t ret = xuart_writes(port, (const char*)data, len);
    xuart_wait_tx_done(port, 10 / portTICK_RATE_MS);
    if (ret > 0) 
    {
		xSemaphoreTake(pppos->mutex, PPPOSMUTEX_TIMEOUT);
    	pppos->rx_count += ret;
		xSemaphoreGive(pppos->mutex);
    }
    return ret;
}

// PPP status callback
//--------------------------------------------------------------
static void ppp_status_cb(ppp_pcb *pcb, int err_code, void *ctx)
{
    pppos_t *pppos = (pppos_t *)ctx;
	struct netif *pppif = ppp_netif(pcb);
	
	switch(err_code) 
    {
		case PPPERR_NONE: 
        {	
			debug("status_cb: Connected");
			#if PPP_IPV4_SUPPORT
			debug("ipaddr    = %s", ipaddr_ntoa(&pppif->ip_addr));
			debug("gateway   = %s", ipaddr_ntoa(&pppif->gw));
			debug("netmask   = %s", ipaddr_ntoa(&pppif->netmask));
			#if PPP_IPV6_SUPPORT
			debug("ip6addr   = %s", ip6addr_ntoa(netif_ip6_addr(pppif, 0)));
			#endif
			#endif
			xSemaphoreTake(pppos->mutex, PPPOSMUTEX_TIMEOUT);
			pppos->status = GSM_STATE_CONNECTED;
			pppos->connection_time = xTaskGetTickCount();
			xSemaphoreGive(pppos->mutex);
			break;
		}
		case PPPERR_PARAM: 
        {
			debug("status_cb: Invalid parameter");
			break;
		}
		case PPPERR_OPEN: 
        {
			debug("status_cb: Unable to open PPP session");
			break;
		}
		case PPPERR_DEVICE: 
        {
			debug("status_cb: Invalid I/O device for PPP");
			break;
		}
		case PPPERR_ALLOC: 
        {
			debug("status_cb: Unable to allocate resources");
			break;
		}
		case PPPERR_USER: 
        {
			/* ppp_free(); -- can be called here */
			debug("status_cb: User interrupt (disconnected)");
			xSemaphoreTake(pppos->mutex, PPPOSMUTEX_TIMEOUT);
			pppos->status = GSM_STATE_DISCONNECTED;
			xSemaphoreGive(pppos->mutex);
			break;
		}
		case PPPERR_CONNECT: 
        {
			debug("status_cb: Connection lost");
			xSemaphoreTake(pppos->mutex, PPPOSMUTEX_TIMEOUT);
			pppos->status = GSM_STATE_DISCONNECTED;
			xSemaphoreGive(pppos->mutex);
			break;
		}
		case PPPERR_AUTHFAIL: 
        {
			debug("status_cb: Failed authentication challenge");
			break;
		}
		case PPPERR_PROTOCOL: 
        {
			debug("status_cb: Failed to meet protocol");
			break;
		}
		case PPPERR_PEERDEAD: 
        {
			debug("status_cb: Connection timeout");
			break;
		}
		case PPPERR_IDLETIMEOUT: 
        {
			debug("status_cb: Idle Timeout");
			break;
		}
		case PPPERR_CONNECTTIME: 
        {
			debug("status_cb: Max connect time reached");
			break;
		}
		case PPPERR_LOOPBACK: 
        {
			debug("status_cb: Loopback detected");
			break;
		}
		default: 
        {
			debug("status_cb: Unknown error code %d", err_code);
			break;
		}
	}
}

pppos_t *pppos_Init(ppposConfig_t *cfg)
{
    /*try make copy of pppos*/
    pppos_t *pppos = pvPortMalloc(sizeof(pppos_t));
    if(!pppos)
	{
		return NULL;
	}
        
	bzero(pppos,sizeof(pppos_t));

    pppos->mutex = xSemaphoreCreateMutex();
    if(!pppos->mutex)
    {
        vPortFree(pppos);
        return NULL;
    }

    pppos->cfg  = cfg->pppos;
    logWindow  = cfg->logWin;
    xTaskCreate(ppposMainTask, "PPPOS", cfg->TaskStack, pppos , cfg->TaskPriority, NULL );

    return pppos;
}


void ppposMainTask( void *pvParameters )
{
    pppos_t *pppos = (pppos_t *)pvParameters;
    pppcfg_t *pppcfg = &pppos->cfg;

    //tcpip_adapter_init();
    debug("PPPOS Start Task.");
	// while(GSM_InitializeBuadRate(pppcfg->dPort)==0)
	// 	vTaskDelay(1000);

    debug("config uart %i in %d",pppcfg->dPort,pppcfg->dBuad);
    bool res = xuart_init(pppcfg->dPort,pppcfg->dBuad);
    if(!res)
    {
        debug("Uart Config Faild");
        vTaskDelete(NULL);
    }
	else
    {
        debug("uart ok.");
    }

    // Allocate receive buffer
    pppos->RcvBuffer = (char*) pvPortMalloc(RECIVE_BUF_SIZE);
    if (pppos->RcvBuffer == NULL) 
	{
		debug("Failed to allocate data buffer.");
    	vTaskDelete(NULL);
    }

    // Set APN from config
    char *PPP_ApnATReq = pvPortMalloc(strlen(pppcfg->APN_name)+24);
    if (PPP_ApnATReq == NULL) 
	{
		debug("Failed to allocate apn name.");
    	vTaskDelete(NULL);
    }
	sprintf(PPP_ApnATReq, "AT+CGDCONT=1,\"IP\",\"%s\"\r\n", pppcfg->APN_name);
	cmd_APN.cmd = PPP_ApnATReq;
	cmd_APN.cmdSize = strlen(PPP_ApnATReq);

    // Disconnect if connected
    force_disconnect(pppcfg->dPort,true); 

    xSemaphoreTake(pppos->mutex, PPPOSMUTEX_TIMEOUT);
    pppos->tx_count = 0;
    pppos->rx_count = 0;
	pppos->status = GSM_STATE_FIRSTINIT;
	xSemaphoreGive(pppos->mutex);

    while(1)
    {
		debug("GSM initialization start");	
		res = false;
		do
		{
			enableAllInitCmd();
			res = GSM_initialize(pppcfg->dPort,&GSM_Init,GSM_InitCmdsSize);
			if(res==true)
				debug("GSM initialized.");
			else
				debug("GSM initializ failed.");
			vTaskDelay(500 / portTICK_PERIOD_MS);
		} while (res==false);

        xSemaphoreTake(pppos->mutex, PPPOSMUTEX_TIMEOUT);
		if (pppos->status == GSM_STATE_FIRSTINIT) 
        {
			xSemaphoreGive(pppos->mutex);
			// ** After first successful initialization create PPP control block
			pppos->ppp = pppapi_pppos_create ( &pppos->ppp_netif,
					ppp_output_callback, ppp_status_cb, pppos);

			if (pppos->ppp == NULL) 
            {
				debug("Error initializing PPPoS");
				break; // end task
			}
			//netif_set_default(&pppos->ppp_netif);
		}
		else 
			xSemaphoreGive(pppos->mutex);

        pppapi_set_default(pppos->ppp);
		pppapi_set_auth(pppos->ppp, PPPAUTHTYPE_PAP, pppcfg->ppp_user, pppcfg->ppp_pass);

        xSemaphoreTake(pppos->mutex, PPPOSMUTEX_TIMEOUT);
		pppos->status = GSM_STATE_IDLE;
		xSemaphoreGive(pppos->mutex);
		pppapi_connect(pppos->ppp, 0);

		while(1)
        {
			// === Check if disconnected ===
			if (pppos->status == GSM_STATE_DISCONNECTED) 
			{
				xSemaphoreGive(pppos->mutex);
				debug("Disconnected, trying again...");
				
				pppapi_close(pppos->ppp, 0);

				pppos->status = GSM_STATE_IDLE;
				vTaskDelay(10000 / portTICK_PERIOD_MS);
				break;
			}
			else xSemaphoreGive(pppos->mutex);

            // === Handle data received from GSM ===
			memset(pppos->RcvBuffer, 0, RECIVE_BUF_SIZE);
			int len = xuart_reads(pppcfg->dPort, (uint8_t*)pppos->RcvBuffer, RECIVE_BUF_SIZE, pdMS_TO_TICKS(100));
			if (len > 0)
			{
				pppos_input_tcpip(pppos->ppp, (u8_t*)pppos->RcvBuffer, len);
				xSemaphoreTake(pppos->mutex, PPPOSMUTEX_TIMEOUT);
			    pppos->tx_count += len;
				xSemaphoreGive(pppos->mutex);
			}
        }
        vTaskDelay(1000);
    }

    vTaskDelay(1000);
    vTaskDelete(NULL);
}


uint32_t GSM_InitializeBuadRate(Serialport_t port)
{
	return 0;
}

static void printAtCommand(char *cmd, int cmdSize)
{
	char buf[cmdSize+2];
	memset(buf, 0, cmdSize+2);
	for (int i=0; i<cmdSize;i++) {
		if ((cmd[i] != 0x00) && ((cmd[i] < 0x20) || (cmd[i] > 0x7F))) buf[i] = '.';
		else buf[i] = cmd[i];
		if (buf[i] == '\0') break;
	}
	AT_debug("AT CMD :[%s]", buf);
}

int gsm_RunCmdWaitResponse(Serialport_t port,char * cmd, char *resp, char * resp1, int cmdSize, int timeout, char **response, int size)
{
	char sresp[256] = {'\0'};
	char data[256] = {'\0'};
    int len, idx = 0, tot = 0, timeoutCnt = 0;
    int res = 1;

	// ** Send command to GSM
	vTaskDelay(100 / portTICK_PERIOD_MS);
	xuart_flush(port);

    /*write the command*/
	if (cmd != NULL) 
    {
		if (cmdSize == -1) 
            cmdSize = strlen(cmd);

        printAtCommand(cmd,cmdSize);
		xuart_writes(port, (const char*)cmd, cmdSize);
		xuart_wait_tx_done(port, 100 / portTICK_RATE_MS);
	}

    /*wait for read response*/
	if (response != NULL) 
    {
		// Read GSM response into buffer
		char *pbuf = *response;
		len = xuart_reads(port, (uint8_t*)data, 256, timeout / portTICK_RATE_MS);
		while (len > 0) 
        {
			if ((tot+len) >= size) 
            {
				char *ptemp = realloc(pbuf, size+512);
				if (ptemp == NULL) return 0;
				size += 512;
				pbuf = ptemp;
			}
			memcpy(pbuf+tot, data, len);
			tot += len;
			response[tot] = '\0';
			len = xuart_reads(port, (uint8_t*)data, 256, 100 / portTICK_RATE_MS);
		}
		*response = pbuf;
		return tot;
	}

    // ** Wait for and check the response
	idx = 0;
	while(1)
	{
		memset(data, 0, 256);
		len = 0;
		len = xuart_reads(port, (uint8_t*)data, 256, 10 / portTICK_RATE_MS);
        
		if (len > 0) 
        {
			for (int i=0; i<len;i++) 
            {
				if (idx < 256) 
                {
					if ((data[i] >= 0x20) && (data[i] < 0x80)) sresp[idx++] = data[i];
					else sresp[idx++] = 0x2e;
				}
			}
			tot += len;
		}
		else 
        {
			if (tot > 0) 
            {
				// Check the response
				if (strstr(sresp, resp) != NULL) 
                {
					AT_debug("AT RESPONSE: [%s]", sresp);
					break;
				}
				else 
                {
					if (resp1 != NULL) 
                    {
						if (strstr(sresp, resp1) != NULL) 
                        {
							
							AT_debug("AT RESPONSE (1): [%s]", sresp);
							res = 2;
							break;
						}
					}
					// no match
					AT_debug("AT BAD RESPONSE: [%s]", sresp);
					
					res = 0;
					break;
				}
			}
		}

		timeoutCnt += 10;
		if (timeoutCnt > timeout) 
        {
			// timeout
			AT_debug("AT: TIMEOUT");
			res = 0;
			break;
		}
	}

	return res;
}


//------------------------------------
void force_disconnect(Serialport_t port,bool rfOff)
{
	int res = gsm_RunCmdWaitResponse(port,"AT\r\n", GSM_OK_Str, NULL, 4, 1000, NULL, 0);
	if (res == 1) 
    {
		if (rfOff) 
        {
			cmd_Reg.timeoutMs = 10000;
			res = gsm_RunCmdWaitResponse(port,"AT+CFUN=4\r\n", GSM_OK_Str, NULL, 11, 10000, NULL, 0); // disable RF function
		}
		return;
	}

	debug("ONLINE, DISCONNECTING...");
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	xuart_flush(port);
	xuart_writes(port, "+++", 3);
    xuart_wait_tx_done(port, 10 / portTICK_RATE_MS);
	vTaskDelay(1100 / portTICK_PERIOD_MS);

	int n = 0;
	res = gsm_RunCmdWaitResponse(port,"ATH\r\n", GSM_OK_Str, "NO CARRIER", 5, 3000, NULL, 0);
	while (res == 0) 
    {
		n++;
		if (n > 10) 
        {	
			debug("STILL CONNECTED.");
			n = 0;
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			xuart_flush(port);
			xuart_writes(port, "+++", 3);
		    xuart_wait_tx_done(port, 10 / portTICK_RATE_MS);
			vTaskDelay(1000 / portTICK_PERIOD_MS);
		}
		vTaskDelay(100 / portTICK_PERIOD_MS);
		res = gsm_RunCmdWaitResponse(port,"ATH\r\n", GSM_OK_Str, "NO CARRIER", 5, 3000, NULL, 0);
	}
	vTaskDelay(100 / portTICK_PERIOD_MS);
	if (rfOff) 
    {
		cmd_Reg.timeoutMs = 10000;
		res = gsm_RunCmdWaitResponse(port,"AT+CFUN=4\r\n", GSM_OK_Str, NULL, 11, 3000, NULL, 0);
	}
	debug("DISCONNECTED.");
}


static void enableAllInitCmd()
{
	for (int idx = 0; idx < GSM_InitCmdsSize; idx++) 
    {
		GSM_Init[idx]->skip = false;
	}
}

static bool GSM_initialize(Serialport_t port,GSM_Cmd **GSM_cmd,size_t cmd_len)
{
	int gsmCmdIter = 0;
	int nfail = 0;
	
	// * GSM Initialization loop
	while(gsmCmdIter < cmd_len)
	{
		/*check command skiped*/
		if (GSM_Init[gsmCmdIter]->skip) 
        {
			gsmCmdIter++;
			continue;
		}

		if (gsm_RunCmdWaitResponse(port,GSM_cmd[gsmCmdIter]->cmd,
				GSM_cmd[gsmCmdIter]->cmdResponseOnOk, NULL,
				GSM_cmd[gsmCmdIter]->cmdSize,
				GSM_cmd[gsmCmdIter]->timeoutMs, NULL, 0) == 0)
		{
			// * No response or not as expected, start from first initialization command
			debug("Wrong response, restarting...");

			nfail++;
			if (nfail > 30)
                return false;

			/*reset command counter*/
			vTaskDelay(pdMS_TO_TICKS(3000));
			gsmCmdIter = 0;
			continue;
		}

		if (GSM_cmd[gsmCmdIter]->delayMs > 0) 
            vTaskDelay(pdMS_TO_TICKS(GSM_cmd[gsmCmdIter]->delayMs));

		/*mark command*/
		GSM_cmd[gsmCmdIter]->skip = true;
		
		if (GSM_cmd[gsmCmdIter] == &cmd_Reg) 
			GSM_cmd[gsmCmdIter]->delayMs = 0;
		
		// Next command
		gsmCmdIter++;
	}
	return true;
}


ppp_statistics *pppos_statistics(pppos_t *ppp,ppp_statistics *stat)
{
	if(xSemaphoreTake(ppp->mutex, PPPOSMUTEX_TIMEOUT)!=pdTRUE)
		return NULL;

	stat->status = ppp->status;
	stat->rx_kb = ppp->rx_count / 1024;
	stat->tx_kb = ppp->tx_count / 1024;
	stat->connection_time = (ppp->connection_time==0) ? 0:
							(xTaskGetTickCount() - ppp->connection_time) / configTICK_RATE_HZ;
	xSemaphoreGive(ppp->mutex);
	return stat;
}