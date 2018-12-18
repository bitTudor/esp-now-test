/*
*********************************************************************************************************
*                                                ESPNOW COMM
*                                    CPU CONFIGURATION & PORT LAYER
*
*                          (c) Copyright 2004-2013; Micrium, Inc.; Weston, FL
*
*               All rights reserved.  Protected by international copyright laws.
*
*               uC/CPU is provided in source form to registered licensees ONLY.  It is
*               illegal to distribute this source code to any third party unless you receive
*               written permission by an authorized Micrium representative.  Knowledge of
*               the source code may NOT be used to develop a similar product.
*
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*
*               You can find our product's user manual, API reference, release notes and
*               more information at https://doc.micrium.com.
*               You can contact us at www.micrium.com.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                           ESPNOW MODULE
*
* Filename      : cpu_core.c
* Version       : V1.30.01
* Programmer(s) : SR
*                 ITJ
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/
#define ESPNOW_MODULE
#include "espnow.h"


/*
*********************************************************************************************************
*
EXTERNAL C LANGUAGE LINKAGE
*
* Note(s) : (1) C++ compilers MUST 'extern'ally declare ALL C function prototypes & variable/object
*
declarations for correct C language linkage.
*********************************************************************************************************
*/
#ifdef __cplusplus
extern
"C" {
/* See Note #1.
*/
#endif


/*
*********************************************************************************************************
*                                             LOCAL DEFINES
*********************************************************************************************************
*/
#define ESPNOW_NODES    2  /* number of nodes - used to get a list with peers mac adresses */

/*
*********************************************************************************************************
*                                            LOCAL CONSTANTS
*********************************************************************************************************
*/
static  const char *ESPNOW_TAG       = "espnow";
static  const char *ESPNOW_DEBUG_TAG = "espnow_debug_info";

/*
*********************************************************************************************************
*                                           LOCAL DATA TYPES
*********************************************************************************************************
*/
typedef  uint8_t peer_list_t[ESP_NOW_ETH_ALEN];


/*
*********************************************************************************************************
*                                            LOCAL TABLES
*********************************************************************************************************
*/
static uint8_t  example_broadcast_mac[ESP_NOW_ETH_ALEN]  = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static uint16_t s_example_espnow_seq[ESPNOW_DATA_MAX]    = { 0, 0 };


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/
static xQueueHandle example_espnow_queue;
static peer_list_t  peer_list[ESPNOW_NODES];

/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/
static void       EspNow_DeInit         (ESPNOW_SEND_PARAM           *send_param);

static esp_err_t  EspNow_EventHandler   (void                        *ctx,
                                         system_event_t              *event);

static void       EspNow_WifiInit       (void);

static void       EspNow_Send_CB        (const uint8_t               *mac_addr,
                                               esp_now_send_status_t  status)

static void       EspNow_Receive_CB     (const uint8_t               *mac_addr,
                                         const uint8_t               *data,
                                               int                    len)

static int        EspNow_DataParse      (uint8_t                     *data,
                                         uint16_t                     data_len,
                                         uint8_t                     *state,
                                         uint16_t                    *seq,
                                         int                         *magic)

static void       EspNow_PrepareData    (ESPNOW_SEND_PARAM           *send_param)
static void       EspNow_Task           (void                        *pvParameter)
/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                           EspNow_EventHandler()
*
* Description : Determine whether a character is an alphabetic character.
*
* Argument(s) : c           Character to examine.
*
* Return(s)   : DEF_YES, if character is     an alphabetic character.
*
*               DEF_NO,     if character is NOT an alphabetic character.
*
* Caller(s)   : Application.
*
* Note(s)     : HAL callback function as prescribed by the U8G2 library.  This callback is invoked
*                   to handle SPI communications.
*********************************************************************************************************
*/
static esp_err_t  EspNow_EventHandler(void          *ctx,
                                     system_event_t *event)
{
    switch(event->event_id) {
         case SYSTEM_EVENT_STA_START:
             ESP_LOGI(ESPNOW_TAG, "WiFi started");
             break;

         default:
             break;
    }

    return ESP_OK;
}


/*
*********************************************************************************************************
*                                           EspNow_WifiInit()
*
* Description : Determine whether a character is an alphabetic character.
*
* Argument(s) : c           Character to examine.
*
* Return(s)   : DEF_YES, if character is     an alphabetic character.
*
*               DEF_NO,     if character is NOT an alphabetic character.
*
* Caller(s)   : Application.
*
* Note(s)     : WiFi should start before using ESPNOW.
*********************************************************************************************************
*/
static void  EspNow_WifiInit(void)
{
    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_init(EspNow_EventHandler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(ESPNOW_WIFI_MODE) );
    ESP_ERROR_CHECK( esp_wifi_start());

    /* In order to simplify example, channel is set after WiFi started.
     * This is not necessary in real application if the two devices have
     * been already on the same channel.
     */
    ESP_ERROR_CHECK( esp_wifi_set_channel(CONFIG_ESPNOW_CHANNEL, 0) );
}



/*
*********************************************************************************************************
*                                           EspNow_Send_CB()
*
* Description : Determine whether a character is an alphabetic character.
*
* Argument(s) : c           Character to examine.
*
* Return(s)   : DEF_YES, if character is     an alphabetic character.
*
*               DEF_NO,     if character is NOT an alphabetic character.
*
* Caller(s)   : Application.
*
* Note(s)     : ESPNOW sending or receiving callback function is called in WiFi task.
*               Users should not do lengthy operations from this task. Instead, post
*               necessary data to a queue and handle it from a lower priority task.
*********************************************************************************************************
*/

/*  */
static void EspNow_Send_CB(const uint8_t              *mac_addr,
                                 esp_now_send_status_t status)
{
    ESPNOW_EVENT evt;
    ESPNOW_SEND_CB_EVENT *send_cb = &evt.info.send_cb;

    if (mac_addr == NULL) {
        ESP_LOGE(ESPNOW_TAG, "Send cb arg error");
        return;
    }

    evt.id = ESPNOW_SEND_CB;
    memcpy(send_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    send_cb->status = status;
    if (xQueueSend(example_espnow_queue, &evt, portMAX_DELAY) != pdTRUE) {
        ESP_LOGW(ESPNOW_TAG, "Send send queue fail");
    }
}



/*
*********************************************************************************************************
*                                           EspNow_Receive_CB()
*
* Description : Determine whether a character is an alphabetic character.
*
* Argument(s) : c           Character to examine.
*
* Return(s)   : DEF_YES, if character is     an alphabetic character.
*
*               DEF_NO,     if character is NOT an alphabetic character.
*
* Caller(s)   : Application.
*
* Note(s)     : ESPNOW sending or receiving callback function is called in WiFi task.
*               Users should not do lengthy operations from this task. Instead, post
*               necessary data to a queue and handle it from a lower priority task.
*********************************************************************************************************
*/
static void  EspNow_Receive_CB(const uint8_t *mac_addr,
                               const uint8_t *data,
                                     int      len)
{
    ESPNOW_EVENT evt;
    ESPNOW_RECEIVE_CB_EVENT *recv_cb = &evt.info.recv_cb;

    if (mac_addr == NULL || data == NULL || len <= 0) {
        ESP_LOGE(ESPNOW_TAG, "Receive cb arg error");
        return;
    }

    evt.id = ESPNOW_RECV_CB;
    memcpy(recv_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    recv_cb->data = malloc(len);
    if (recv_cb->data == NULL) {
        ESP_LOGE(ESPNOW_TAG, "Malloc receive data fail");
        return;
    }
    memcpy(recv_cb->data, data, len);
    recv_cb->data_len = len;
    if (xQueueSend(example_espnow_queue, &evt, portMAX_DELAY) != pdTRUE) {
        ESP_LOGW(ESPNOW_TAG, "Send receive queue fail");
        free(recv_cb->data);
    }
}


/*
*********************************************************************************************************
*                                           EspNow_Receive_CB()
*
* Description : Determine whether a character is an alphabetic character.
*
* Argument(s) : c           Character to examine.
*
* Return(s)   : DEF_YES, if character is     an alphabetic character.
*
*               DEF_NO,     if character is NOT an alphabetic character.
*
* Caller(s)   : Application.
*
* Note(s)     : Parse received ESPNOW data.
*********************************************************************************************************
*/
int  EspNow_DataParse(uint8_t  *data,
                      uint16_t  data_len,
                      uint8_t  *state,
                      uint16_t *seq,
                      int      *magic)
{
    ESPNOW_DATA *buf = (ESPNOW_DATA *)data;
    uint16_t crc, crc_cal = 0;

    if (data_len < sizeof(ESPNOW_DATA)) {
        ESP_LOGE(ESPNOW_TAG, "Receive ESPNOW data too short, len:%d", data_len);
        return -1;
    }

    *state = buf->state;
    *seq = buf->seq_num;
    *magic = buf->magic;
    crc = buf->crc;
    buf->crc = 0;
    crc_cal = crc16_le(UINT16_MAX, (uint8_t const *)buf, data_len);

    if (crc_cal == crc) {
        return buf->type;
    }

    return -1;
}


/*
*********************************************************************************************************
*                                           EspNow_PrepareData()
*
* Description : Determine whether a character is an alphabetic character.
*
* Argument(s) : c           Character to examine.
*
* Return(s)   : DEF_YES, if character is     an alphabetic character.
*
*               DEF_NO,     if character is NOT an alphabetic character.
*
* Caller(s)   : Application.
*
* Note(s)     : Prepare ESPNOW data to be sent.
*********************************************************************************************************
*/
void EspNow_PrepareData(ESPNOW_SEND_PARAM *send_param)
{
    ESPNOW_DATA *buf = (ESPNOW_DATA *)send_param->buffer;
    int i = 0;

    assert(send_param->len >= sizeof(ESPNOW_DATA));

    buf->type = ESPNOW_IS_BROADCAST_ADDR(send_param->dest_mac) ? ESPNOW_BROADCAST_DATA : ESPNOW_UNICAST_DATA;
    buf->state = send_param->state;
    buf->seq_num = s_example_espnow_seq[buf->type]++;
    buf->crc = 0;
    buf->magic = send_param->magic;
    for (i = 0; i < send_param->len - sizeof(ESPNOW_DATA); i++) {
      //  buf->payload[i] = 'T'; //(uint8_t)esp_random();
    	buf->payload[i] = (uint8_t)esp_random();
    }
  //  ESP_LOGW(ESPNOW_DEBUG_TAG, "prepare data sent:  %s", buf->payload);
    buf->crc = crc16_le(UINT16_MAX, (uint8_t const *)buf, send_param->len);
}


/*
*********************************************************************************************************
*                                           EspNow_DeInit()
*
* Description : Determine whether a character is an alphabetic character.
*
* Argument(s) : c           Character to examine.
*
* Return(s)   : DEF_YES, if character is     an alphabetic character.
*
*               DEF_NO,     if character is NOT an alphabetic character.
*
* Caller(s)   : Application.
*
* Note(s)     : Prepare ESPNOW data to be sent.
*********************************************************************************************************
*/
static void  EspNow_DeInit(ESPNOW_SEND_PARAM *send_param)
{
    free(send_param->buffer);
    free(send_param);
    vSemaphoreDelete(example_espnow_queue);
    esp_now_deinit();
}



/*
*********************************************************************************************************
*                                           EspNow_Task()
*
* Description : Determine whether a character is an alphabetic character.
*
* Argument(s) : c           Character to examine.
*
* Return(s)   : DEF_YES, if character is     an alphabetic character.
*
*               DEF_NO,     if character is NOT an alphabetic character.
*
* Caller(s)   : Application.
*
* Note(s)     : Prepare ESPNOW data to be sent.
*********************************************************************************************************
*/
static void EspNow_Task(void *pvParameter)
{
    ESPNOW_EVENT evt;
    uint8_t recv_state = 0;
    uint16_t recv_seq  = 0;
    int recv_magic     = 0;
    bool is_broadcast  = false;
    int ret, t;

    vTaskDelay(5000 / portTICK_RATE_MS);
    ESP_LOGI(ESPNOW_TAG, "Start sending broadcast data");

    /* Start sending broadcast ESPNOW data. */
    ESPNOW_SEND_PARAM *send_param = (ESPNOW_SEND_PARAM *)pvParameter;
    ESP_LOGW(ESPNOW_DEBUG_TAG, "send data - intrare in task");
    if (esp_now_send(send_param->dest_mac, send_param->buffer, send_param->len) != ESP_OK) {
        ESP_LOGE(ESPNOW_TAG, "Send error");
        EspNow_DeInit(send_param);
        vTaskDelete(NULL);
    }

    while (xQueueReceive(example_espnow_queue, &evt, portMAX_DELAY) == pdTRUE) {
        switch (evt.id) {
            case ESPNOW_SEND_CB:
            {
                ESPNOW_SEND_CB_EVENT *send_cb = &evt.info.send_cb;
                is_broadcast = ESPNOW_IS_BROADCAST_ADDR(send_cb->mac_addr);

                ESP_LOGD(ESPNOW_TAG, "Send data to "MACSTR", status1: %d", MAC2STR(send_cb->mac_addr), send_cb->status);

                if (is_broadcast && (send_param->broadcast == false)) {
                	ESP_LOGW(ESPNOW_DEBUG_TAG, "Broadcast break - warning  GALBEN");
                	ESP_LOGE(ESPNOW_DEBUG_TAG, "Broadcast break -error ROSU");
                	                    break;
                }

                if (!is_broadcast) {
                    send_param->count--;
                    /**********************************************************************/
                    esp_now_peer_num_t *peer_number = malloc(sizeof(esp_now_peer_num_t));
                    if (peer_number == NULL) {
                        ESP_LOGE(ESPNOW_TAG, "Malloc peer number fail");
                        EspNow_DeInit(send_param);
                        vTaskDelete(NULL);
                    }
                    memset(peer_number, 0, sizeof(esp_now_peer_num_t));
                    ESP_ERROR_CHECK( esp_now_get_peer_num(peer_number));
                    ESP_LOGW(ESPNOW_DEBUG_TAG, "GET PEER NUMBER - %d",peer_number->total_num);

                    /*=================================================================*/
                    esp_now_peer_info_t *peer_info = malloc(sizeof(esp_now_peer_info_t));
                    if (peer_info == NULL) {
                        ESP_LOGE(ESPNOW_TAG, "Malloc peer information fail");
                        EspNow_DeInit(send_param);
                        vTaskDelete(NULL);
                    }
                    memset(peer_info, 0, sizeof(esp_now_peer_info_t));
                    esp_err_t e = 0;
                    int n = 0;
                    for (e = esp_now_fetch_peer(1,peer_info); e == ESP_OK; e = esp_now_fetch_peer(0,peer_info))
                    {
                    	memcpy(peer_list[n], peer_info->peer_addr, 6);
                    	ESP_LOGE(ESPNOW_TAG, "Adresa peer %d:  "MACSTR" ", n,  MAC2STR(peer_info->peer_addr));
                    	ESP_LOGE(ESPNOW_TAG, "Adresa peerdin peer list %d:  "MACSTR" ", n,  MAC2STR(peer_list[n]));
                    	++n;

                    }

                    /* Delay a while before sending the next data. */
                    if (send_param->delay > 0) {
                        vTaskDelay(send_param->delay/portTICK_RATE_MS);
                    }

                    ESP_LOGI(ESPNOW_TAG, "send data to "MACSTR"", MAC2STR(peer_list[0]));

                    memcpy(send_param->dest_mac, peer_list[0], ESP_NOW_ETH_ALEN);
                    EspNow_PrepareData(send_param);
                    ESP_LOGW(ESPNOW_DEBUG_TAG, "send data in send");
                    /* Send the next data after the previous data is sent. */
                    if (esp_now_send(send_param->dest_mac, send_param->buffer, send_param->len) != ESP_OK) {
                        ESP_LOGE(ESPNOW_TAG, "Send error");
                        EspNow_DeInit(send_param);
                        vTaskDelete(NULL);
                    }

                    /* Delay a while before sending the next data. */
                    if (send_param->delay > 0) {
                        vTaskDelay(send_param->delay/portTICK_RATE_MS);
                    }

                    ESP_LOGI(ESPNOW_TAG, "send data to "MACSTR"", MAC2STR(peer_list[1]));

                    memcpy(send_param->dest_mac, peer_list[1], ESP_NOW_ETH_ALEN);
                    EspNow_PrepareData(send_param);
                    ESP_LOGW(ESPNOW_DEBUG_TAG, "send data in send");
                    /* Send the next data after the previous data is sent. */
                    if (esp_now_send(send_param->dest_mac, send_param->buffer, send_param->len) != ESP_OK) {
                        ESP_LOGE(ESPNOW_TAG, "Send error");
                        EspNow_DeInit(send_param);
                        vTaskDelete(NULL);
                    }

                    	/*
                    for (t = 0; t < peer_number->total_num; t++)
                    {
                    	ESP_ERROR_CHECK( esp_now_fetch_peer(0,peer_info));
                        ESP_LOGE(ESPNOW_TAG, "Adresa peer %d:  "MACSTR" ", t,  MAC2STR(peer_info->peer_addr));
                        // ESP_ERROR_CHECK( esp_now_fetch_peer(0,peer_info));
                    }
                    // ESP_LOGW(ESPNOW_DEBUG_TAG, "GET PEER NUMBER - %d",peer_number->total_num);
*/
                    free(peer_number);
                    free(peer_info);

                     /**********************************************************************/
                    if (send_param->count == 0) {
                        ESP_LOGI(ESPNOW_TAG, "Send done");
                        EspNow_DeInit(send_param);
                        vTaskDelete(NULL);
                    }
                }

                /* Delay a while before sending the next data. */
                if (send_param->delay > 0) {
                    vTaskDelay(send_param->delay/portTICK_RATE_MS);
                }

                ESP_LOGI(ESPNOW_TAG, "send data to "MACSTR"", MAC2STR(send_cb->mac_addr));

                memcpy(send_param->dest_mac, send_cb->mac_addr, ESP_NOW_ETH_ALEN);
                EspNow_PrepareData(send_param);
                ESP_LOGW(ESPNOW_DEBUG_TAG, "send data in send");
                /* Send the next data after the previous data is sent. */
                if (esp_now_send(send_param->dest_mac, send_param->buffer, send_param->len) != ESP_OK) {
                    ESP_LOGE(ESPNOW_TAG, "Send error");
                    EspNow_DeInit(send_param);
                    vTaskDelete(NULL);
                }
                break;
            }
            case ESPNOW_RECV_CB:
            {
                ESPNOW_RECEIVE_CB_EVENT *recv_cb = &evt.info.recv_cb;

                ret = EspNow_DataParse(recv_cb->data, recv_cb->data_len, &recv_state, &recv_seq, &recv_magic);
                ESP_LOGE(ESPNOW_DEBUG_TAG, "inainte de  - flag 1");
                free(recv_cb->data);
                if (ret == ESPNOW_BROADCAST_DATA) {
                    ESP_LOGI(ESPNOW_TAG, "Receive %dth broadcast data from: "MACSTR", len: %d", recv_seq, MAC2STR(recv_cb->mac_addr), recv_cb->data_len);

                    /* If MAC address does not exist in peer list, add it to peer list. */
                    if (esp_now_is_peer_exist(recv_cb->mac_addr) == false) {
                        esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
                        if (peer == NULL) {
                            ESP_LOGE(ESPNOW_TAG, "Malloc peer information fail");
                            EspNow_DeInit(send_param);
                            vTaskDelete(NULL);
                        }
                        memset(peer, 0, sizeof(esp_now_peer_info_t));
                        peer->channel = CONFIG_ESPNOW_CHANNEL;
                        peer->ifidx = ESPNOW_WIFI_IF;
                        peer->encrypt = true;
                        memcpy(peer->lmk, CONFIG_ESPNOW_LMK, ESP_NOW_KEY_LEN);
                        memcpy(peer->peer_addr, recv_cb->mac_addr, ESP_NOW_ETH_ALEN);
                        ESP_ERROR_CHECK( esp_now_add_peer(peer) );
                        free(peer);
                    }

                    /* Indicates that the device has received broadcast ESPNOW data. */
                    if (send_param->state == 0) {
                        send_param->state = 1;
                        ESP_LOGE(ESPNOW_DEBUG_TAG, "a primit broadcast data - flag 1");
                    }

                    /* If receive broadcast ESPNOW data which indicates that the other device has received
                     * broadcast ESPNOW data and the local magic number is bigger than that in the received
                     * broadcast ESPNOW data, stop sending broadcast ESPNOW data and start sending unicast
                     * ESPNOW data.
                     */
                    if (recv_state == 1) {
                        /* The device which has the bigger magic number sends ESPNOW data, the other one
                         * receives ESPNOW data.
                         */
                        if (send_param->unicast == false && send_param->magic >= recv_magic) {
                    	    ESP_LOGI(ESPNOW_TAG, "Start sending unicast data");
                    	    ESP_LOGI(ESPNOW_TAG, "send data to "MACSTR"", MAC2STR(recv_cb->mac_addr));

                    	    /* Start sending unicast ESPNOW data. */
                    	    ESP_LOGW(ESPNOW_DEBUG_TAG, "send data in receive");
                    	    ESP_LOGE(ESPNOW_TAG, "send data to "MACSTR"", MAC2STR(send_param->dest_mac));
                            memcpy(send_param->dest_mac, recv_cb->mac_addr, ESP_NOW_ETH_ALEN);
                            EspNow_PrepareData(send_param);
                            if (esp_now_send(send_param->dest_mac, send_param->buffer, send_param->len) != ESP_OK) {
                                ESP_LOGE(ESPNOW_TAG, "Send error");
                                EspNow_DeInit(send_param);
                                vTaskDelete(NULL);
                            }
                            else {
                            	ESP_LOGW(ESPNOW_DEBUG_TAG, "send data in receive - else");
                                send_param->broadcast = false;
                                send_param->unicast = true;
                            }
                        }
                    }
                }
                else if (ret == ESPNOW_UNICAST_DATA) {
                    ESP_LOGI(ESPNOW_TAG, "Receive %dth unicast data from: "MACSTR", len: %d", recv_seq, MAC2STR(recv_cb->mac_addr), recv_cb->data_len);

                    /* If receive unicast ESPNOW data, also stop sending broadcast ESPNOW data. */
                    send_param->broadcast = false;
                }
                else {
                    ESP_LOGI(ESPNOW_TAG, "Receive error data from: "MACSTR"", MAC2STR(recv_cb->mac_addr));
                }
                break;
            }
            default:
                ESP_LOGE(ESPNOW_TAG, "Callback type error: %d", evt.id);
                break;
        }
    }
}
/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                           U8g2Hal_SpiByte_CB()
*
* Description : Determine whether a character is an alphabetic character.
*
* Argument(s) : c           Character to examine.
*
* Return(s)   : DEF_YES, if character is     an alphabetic character.
*
*               DEF_NO,     if character is NOT an alphabetic character.
*
* Caller(s)   : Application.
*
* Note(s)     : HAL callback function as prescribed by the U8G2 library.  This callback is invoked
*                   to handle callbacks for GPIO and delay functions.
*********************************************************************************************************
*/
esp_err_t  EspNow_Init(void)
{
    ESPNOW_SEND_PARAM *send_param;

    example_espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(ESPNOW_EVENT));
    if (example_espnow_queue == NULL) {
        ESP_LOGE(ESPNOW_TAG, "Create mutex fail");
        return ESP_FAIL;
    }

    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(EspNow_Send_CB) );
    ESP_ERROR_CHECK( esp_now_register_recv_cb(EspNow_Receive_CB) );

    /* Set primary master key. */
    ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) );

    /* Add broadcast peer information to peer list. */
    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL) {
        ESP_LOGE(ESPNOW_TAG, "Malloc peer information fail");
        vSemaphoreDelete(example_espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = CONFIG_ESPNOW_CHANNEL;
    peer->ifidx = ESPNOW_WIFI_IF;
    peer->encrypt = false;
    memcpy(peer->peer_addr, example_broadcast_mac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK( esp_now_add_peer(peer) );
    ESP_LOGE(ESPNOW_TAG, "Adresa peer :  "MACSTR" ",  MAC2STR(peer->peer_addr));
    free(peer);

    /* Initialize sending parameters. */
    send_param = malloc(sizeof(ESPNOW_SEND_PARAM));
    memset(send_param, 0, sizeof(ESPNOW_SEND_PARAM));
    if (send_param == NULL) {
        ESP_LOGE(ESPNOW_TAG, "Malloc send parameter fail");
        vSemaphoreDelete(example_espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }
    send_param->unicast = false;
    send_param->broadcast = true;
    send_param->state = 0;
    send_param->magic = 0xFFFFFFFF; //esp_random();
    send_param->count = CONFIG_ESPNOW_SEND_COUNT;
    send_param->delay = CONFIG_ESPNOW_SEND_DELAY;
    send_param->len = CONFIG_ESPNOW_SEND_LEN;
    send_param->buffer = malloc(CONFIG_ESPNOW_SEND_LEN);
    if (send_param->buffer == NULL) {
        ESP_LOGE(ESPNOW_TAG, "Malloc send buffer fail");
        free(send_param);
        vSemaphoreDelete(example_espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }
    memcpy(send_param->dest_mac, example_broadcast_mac, ESP_NOW_ETH_ALEN);
    EspNow_PrepareData(send_param);

    //ESP_LOGW(ESPNOW_DEBUG_TAG, "init data sent:  %.20s", (uint8_t*)(send_param->buffer));
  //  ESP_LOGW(ESPNOW_DEBUG_TAG, "init data sent:  %.d", (int)send_param->buffer);
    char *TmpStr = "TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT";
    //strncpy(TmpStr,(char *)(send_param->buffer),20);
    ESPNOW_DATA *buf = (ESPNOW_DATA *)send_param->buffer;
    ESP_LOGW(ESPNOW_DEBUG_TAG, "init payload data sent:  %.10s", buf->payload);
  //  ESP_LOGW(ESPNOW_DEBUG_TAG, "init data sent:  %s", TmpStr);
    ESP_LOGW(ESPNOW_DEBUG_TAG, "prepare data sent:  %c", buf->payload[0]);

   memcpy(send_param->buffer, TmpStr, 20);
   ESP_LOGW(ESPNOW_DEBUG_TAG, "init buffer data sent:  %.10s", send_param->buffer);

    xTaskCreate(EspNow_Task, "espnow_task", 2048, send_param, 4, NULL);

    return ESP_OK;
}





/*
*********************************************************************************************************
*
EXTERNAL C LANGUAGE LINKAGE END
*********************************************************************************************************
*/
#ifdef __cplusplus
}
#endif
