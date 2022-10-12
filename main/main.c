/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdint.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"
#include "debug/debugtask.h"
#include "app_config.h"
#include "esp_log.h"
#include <stdarg.h>
#include "RTOSAnalysis/RTOSAnalysis.h"
#include "pppos/pppos.h"

#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_eth.h"


#include "lwip/err.h"
#include "lwip/sys.h"
#include "string.h"

#include "ping/ping_sock.h"
#include "lwip/inet.h"
#include "lwip/ip4_addr.h"
#include "ping/ping_sock.h"
#include "sdkconfig.h"

#if !IP_NAPT
#error "IP_NAPT must be defined"
#endif
#include "lwip/lwip_napt.h"


#define r_led   GPIO_NUM_13
#define b_led   GPIO_NUM_15

int log_zMonitor(const char *fmt, va_list args);

#define debug(...)      xdebugf(xWindow13,__VA_ARGS__)
/* The examples use WiFi configuration that you can set via project configuration menu.
   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      "uRouter"
#define EXAMPLE_ESP_WIFI_PASS      "1234567890"
#define EXAMPLE_ESP_WIFI_CHANNEL   1
#define EXAMPLE_MAX_STA_CONN       32
esp_netif_t* wifiAP;
uint32_t my_ap_ip;
pppos_t *pppos = NULL;

static esp_eth_handle_t s_eth_handle = NULL;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) 
    {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        debug("station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } 
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) 
    {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        debug("station "MACSTR" leave, AID=%d", MAC2STR(event->mac), event->aid);
    }
}

void wifi_init_softap(void)
{
    wifiAP = esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    my_ap_ip = ipaddr_addr("192.168.4.1");

    esp_netif_ip_info_t ipInfo_ap;
    ipInfo_ap.ip.addr = my_ap_ip;
    ipInfo_ap.gw.addr = my_ap_ip;
    IP4_ADDR(&ipInfo_ap.netmask, 255,255,255,0);
    esp_netif_dhcps_stop(wifiAP); // stop before setting ip WifiAP
    esp_netif_set_ip_info(wifiAP, &ipInfo_ap);
    esp_netif_dhcps_start(wifiAP);


    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    debug("wifi_init_softap finished. SSID:%s password:%s channel:%d", EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
}

#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
static void cmd_ping_on_ping_success(esp_ping_handle_t hdl, void *args)
{
    uint8_t ttl;
    uint16_t seqno;
    uint32_t elapsed_time, recv_len;
    ip_addr_t target_addr;
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TTL, &ttl, sizeof(ttl));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    esp_ping_get_profile(hdl, ESP_PING_PROF_SIZE, &recv_len, sizeof(recv_len));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time));
    debug("%d bytes from %s icmp_seq=%d ttl=%d time=%d ms",
           recv_len, ipaddr_ntoa((ip_addr_t*)&target_addr), seqno, ttl, elapsed_time);
}

static void cmd_ping_on_ping_timeout(esp_ping_handle_t hdl, void *args)
{
    uint16_t seqno;
    ip_addr_t target_addr;
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    debug("From %s icmp_seq=%d timeout",ipaddr_ntoa((ip_addr_t*)&target_addr), seqno);
}

static void cmd_ping_on_ping_end(esp_ping_handle_t hdl, void *args)
{
    ip_addr_t target_addr;
    uint32_t transmitted;
    uint32_t received;
    uint32_t total_time_ms;
    esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
    esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &received, sizeof(received));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    esp_ping_get_profile(hdl, ESP_PING_PROF_DURATION, &total_time_ms, sizeof(total_time_ms));
    uint32_t loss = (uint32_t)((1 - ((float)received) / transmitted) * 100);
   
    debug("\n--- %s ping statistics ---", inet_ntoa(*ip_2_ip4(&target_addr)));
   
    debug("%d packets transmitted, %d received, %d%% packet loss, time %dms",
           transmitted, received, loss, total_time_ms);
    // delete the ping sessions, so that we clean up all resources and can create a new ping session
    // we don't have to call delete function in the callback, instead we can call delete function from other tasks
    esp_ping_delete_session(hdl);
}

static int do_ping_cmd(void)
{
    esp_ping_config_t config = ESP_PING_DEFAULT_CONFIG();


    
    config.timeout_ms = 5000;
    config.interval_ms = 1000;
    config.data_size = 1024;
    config.count = 10;
    config.tos = 0;


    // parse IP address
    ip_addr_t target_addr;
    memset(&target_addr, 0, sizeof(target_addr));
    {
        struct addrinfo hint;
        struct addrinfo *res = NULL;
        memset(&hint, 0, sizeof(hint));
        /* convert ip4 string or hostname to ip4 or ip6 address */
        if (getaddrinfo("4.2.2.4", NULL, &hint, &res) != 0) 
        {
            debug("ping: unknown host %s\n", "4.2.2.4");
            return 1;
        }
        if (res->ai_family == AF_INET) {
            struct in_addr addr4 = ((struct sockaddr_in *) (res->ai_addr))->sin_addr;
            inet_addr_to_ip4addr(ip_2_ip4(&target_addr), &addr4);
        } else {
            struct in6_addr addr6 = ((struct sockaddr_in6 *) (res->ai_addr))->sin6_addr;
            inet6_addr_to_ip6addr(ip_2_ip6(&target_addr), &addr6);
        }
        freeaddrinfo(res);
    }
    config.target_addr = target_addr;

    /* set callback functions */
    esp_ping_callbacks_t cbs = {
        .cb_args = NULL,
        .on_ping_success = cmd_ping_on_ping_success,
        .on_ping_timeout = cmd_ping_on_ping_timeout,
        .on_ping_end = cmd_ping_on_ping_end
    };
    esp_ping_handle_t ping;
    esp_ping_new_session(&config, &cbs, &ping);
    esp_ping_start(ping);

    return 0;
}


/** Event handler for Ethernet events */
static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI("ETH", "Ethernet Link Up :)");
        ESP_LOGI("ETH", "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        
        /*try to reset default gateway*/
        if(pppos->ppp!=NULL && 	netif_is_up(pppos->ppp->netif))
        {
            ESP_LOGI("ETH","ppp Connection connected :)");
            pppapi_set_default(pppos->ppp);
        }
        
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI("ETH", "Ethernet Link Down");
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI("ETH", "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI("ETH", "Ethernet Stopped");
        break;
    default:
        break;
    }
}

/*https://github.com/espressif/esp-idf/issues/4086*/
static void initialize_ethernet(void)
{
    /*it's important for nat*/
    my_ap_ip = ipaddr_addr("192.168.4.1");

    const esp_netif_ip_info_t my_eth_ip = {
        .ip = { .addr = ESP_IP4TOADDR( 192, 168, 4, 1) },
        .gw = { .addr = ESP_IP4TOADDR( 192, 168, 4, 1) },
        .netmask = { .addr = ESP_IP4TOADDR( 255, 255, 255, 0) },
    };

    const esp_netif_inherent_config_t eth_behav_cfg = {
        .get_ip_event = IP_EVENT_ETH_GOT_IP,
        .lost_ip_event = 0,
        .flags = ESP_NETIF_DHCP_SERVER | ESP_NETIF_FLAG_AUTOUP,
        .ip_info = (esp_netif_ip_info_t*)& my_eth_ip,
        .if_key = "ETHDHCPS",
        .if_desc = "eth",
        .route_prio = 50
    };
    // Create new default instance of esp-netif for Ethernet
    //esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_config_t cfg = { .base = & eth_behav_cfg, .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH };
    esp_netif_t *eth_netif = esp_netif_new(&cfg);


    
    // Init MAC and PHY configs to default
    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();

    phy_config.phy_addr = 1;
    phy_config.reset_gpio_num = 5;
    mac_config.smi_mdc_gpio_num = 23;
    mac_config.smi_mdio_gpio_num = 18;

    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&mac_config);
    esp_eth_phy_t *phy = esp_eth_phy_new_lan87xx(&phy_config);
    esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);
    esp_eth_handle_t eth_handle = NULL;
    ESP_ERROR_CHECK(esp_eth_driver_install(&config, &eth_handle));
    /* attach Ethernet driver to TCP/IP stack */
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)));
    esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL); // register Ethernet event handler (to deal with user specific stuffs when event like link up/down happened)

    esp_eth_start(eth_handle); // start Ethernet driver state machine

    esp_netif_ip_info_t ipInfo_ap;
    ipInfo_ap.ip.addr = my_ap_ip;
    ipInfo_ap.gw.addr = my_ap_ip;
    IP4_ADDR(&ipInfo_ap.netmask, 255,255,255,0);
    
    esp_netif_dhcps_stop(eth_netif); // stop before setting ip WifiAP
    esp_netif_set_ip_info(eth_netif, &ipInfo_ap);
    ip_addr_t dnsserver;
    IP_ADDR4(&dnsserver, 8, 8, 8, 8);
    dhcps_dns_setserver(&dnsserver);
    dhcps_offer_t opt_val = OFFER_DNS; // supply a dns server via dhcps
    esp_netif_dhcps_option(eth_netif,ESP_NETIF_OP_SET,ESP_NETIF_DOMAIN_NAME_SERVER,&opt_val,sizeof(dhcps_offer_t));

    esp_netif_dhcps_start(eth_netif);

}

/*
https://github.com/mtnbkr88/ESP32LocalPortForwarder
https://www.hackster.io/simon-vavpotic/esp32-wifi-to-ethernet-bridge-a2adaa
*/
void app_main(void)
{
    /*init zMonitor console*/
    debug_t dbg;
    dbg.TaskStack = Debug_Task_Stak;
    dbg.TaskPriority = Debug_PRIORITY;
    dbg.QueueLen = Debug_QItem;
    dbg.ItemLen = Debug_QSize;
    dbg.dPort = Debug_Port;
    dbg.dBuad = Debug_Buad;
    DebugTask_Init(&dbg);

    /*redirect Esp Log To zMonitor*/
    esp_log_set_vprintf(&log_zMonitor);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize TCP/IP network interface (should be called only once in application)
    ESP_ERROR_CHECK(esp_netif_init());
    // Create default event loop that running in background
    ESP_ERROR_CHECK(esp_event_loop_create_default());


    /*init RTOS Analysis*/
    RTOSAnalysis_Init(RTOSAnalysis_TASK_Stak,RTOSAnalysis_PRIORITY,RTOSAnalysis_zWindow);

    /*init PPPOS */
    ppposConfig_t pcfg  = {
        .TaskPriority = PPPOS_PRIORITY,
        .TaskStack = PPPOS_TASK_Stak,
        .logWin    = PPPOS_zWindow,
        .pppos = {
            .dPort = PPPOS_Port,
            .dBuad = PPPOS_Buad,
            .ppp_pass = "0000",
            .ppp_user = "irancell",
            .APN_name = "mtnirancell"
        }
    };
    pppos = pppos_Init(&pcfg);

    //wifi_init_softap();
    initialize_ethernet();


    gpio_config_t io_conf = {};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = (1<<GPIO_NUM_2) | (1<<r_led) | (1<<b_led);
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    int counter = 0;

    int  pining = 0;
    ppp_statistics p_stat;
    while (true)
    {
        ppp_statistics *stat = pppos_statistics(pppos,&p_stat);
        uint32_t Status_Led = r_led;
        if(stat!=NULL && stat->status == GSM_STATE_CONNECTED)
        {
            Status_Led = b_led;
            if(pining==0)
            {
                pining = 1;
                ip_napt_enable(my_ap_ip, 1);
                ESP_LOGI("NAT", "NAT is enabled");
                debug("Start Ping");
                do_ping_cmd();
            }
            xdebugf(xWindow23,"Live:%d\tRx: %dkb\tTx: %dkb",stat->connection_time,stat->rx_kb,stat->tx_kb);    
        }
            
        

        xdebugf(xWindow12,"My Live is %i",counter++);
        
        //gpio_set_level(GPIO_NUM_2,1);
        gpio_set_level(Status_Led,1);
        vTaskDelay(pdMS_TO_TICKS(100));
        //gpio_set_level(GPIO_NUM_2,0);
        gpio_set_level(Status_Led,0);
        vTaskDelay(pdMS_TO_TICKS(1000));


        /*get list of interface*/
        // Create an esp_netif pointer to store current interface
        esp_netif_t *ifscan = esp_netif_next (NULL);

        // Stores the unique interface descriptor, such as "PP1", etc
        char ifdesc[7];
        ifdesc[6] = 0;  // Ensure null terminated string

        while (ifscan != NULL)
        {
            esp_netif_get_netif_impl_name (ifscan, ifdesc);
            xdebugf (xWindow14, "IF NAME: '%s'", ifdesc);
            // Get the next interface
            ifscan = esp_netif_next (ifscan);
        }

        //gpio_set_level(45,1);
    }
}



int log_zMonitor(const char *fmt, va_list args) 
{
    char dparam[Debug_QSize];
    vsnprintf(dparam,Debug_QSize,fmt,args);
    return xdebugf(xWindow31,dparam);
}