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
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                          ESPNOW COMM MODULE
*
* Filename      : cpu_core.h
* Version       : V1.30.01
* Programmer(s) : SR
*                 ITJ
*********************************************************************************************************
* Note(s)       : (1) Assumes the following versions (or more recent) of software modules are included in
*                     the project build :
*
*                     (a) uC/LIB V1.35.00
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                               MODULE
*
* Note(s) : (1) This core CPU header file is protected from multiple pre-processor inclusion through use of
*               the  core CPU module present pre-processor macro definition.
*********************************************************************************************************
*/

#ifndef ESPNOW_H_INCLUDED
#define ESPNOW_H_INCLUDED

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include "esp_event_loop.h"
#include "tcpip_adapter.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_now.h"
#include "rom/ets_sys.h"
#include "crc.h"
//#include "rom/crc.h"

/*
*********************************************************************************************************
*                                               EXTERNS
*********************************************************************************************************
*/
#ifdef   ESPNOW_MODULE
#define  ESPNOW_EXT
#else
#define  ESPNOW_EXT  extern
#endif

/*
*********************************************************************************************************
*                                      LANGUAGE SUPPORT DEFINES
*********************************************************************************************************
*/

#ifdef __cplusplus
 extern "C" {
#endif

 /*
 *********************************************************************************************************
 *                                               CONSTANTS
 *********************************************************************************************************
 */






 /*
 *********************************************************************************************************
 *                                                DEFINES
 *********************************************************************************************************
 */
#if CONFIG_STATION_MODE                        /* ESPNOW can work in both station and softap mode. It is configured in menuconfig. */
#define ESPNOW_WIFI_MODE     WIFI_MODE_STA
#define ESPNOW_WIFI_IF       ESP_IF_WIFI_STA
#else
#define ESPNOW_WIFI_MODE     WIFI_MODE_AP
#define ESPNOW_WIFI_IF       ESP_IF_WIFI_AP
#endif

#define ESPNOW_QUEUE_SIZE    6

 /*
 *********************************************************************************************************
 *                                            GLOBAL MACRO'S
 *********************************************************************************************************
 */
#define ESPNOW_IS_BROADCAST_ADDR(addr) (memcmp(addr, example_broadcast_mac, ESP_NOW_ETH_ALEN) == 0)

 /*
 *********************************************************************************************************
 *                                          GLOBAL DATA TYPES
 *********************************************************************************************************
 */
typedef enum {
    ESPNOW_SEND_CB,
    ESPNOW_RECV_CB,
} ESPNOW_EVENT_ID;


typedef struct {
    uint8_t                mac_addr[ESP_NOW_ETH_ALEN];
    esp_now_send_status_t  status;
} ESPNOW_SEND_CB_EVENT;


typedef struct {
    uint8_t  mac_addr[ESP_NOW_ETH_ALEN];
    uint8_t *data;
    int      data_len;
} ESPNOW_RECEIVE_CB_EVENT;


typedef union {
    ESPNOW_SEND_CB_EVENT     send_cb;
    ESPNOW_RECEIVE_CB_EVENT  recv_cb;
} ESPNOW_EVENT_INFO;


typedef struct {                                  /* When ESPNOW sending or receiving callback function is called, post event to ESPNOW task. */
    ESPNOW_EVENT_ID    id;
    ESPNOW_EVENT_INFO  info;
} ESPNOW_EVENT;


enum {
    ESPNOW_BROADCAST_DATA,
    ESPNOW_UNICAST_DATA,
    ESPNOW_DATA_MAX,
};


typedef struct {                              /* User defined field of ESPNOW data in this example.                                */
    uint8_t   type;                           /* Broadcast or unicast ESPNOW data.                                                 */
    uint8_t   state;                          /* Indicate that if has received broadcast ESPNOW data or not.                       */
    uint16_t  seq_num;                        /* Sequence number of ESPNOW data.                                                   */
    uint16_t  crc;                            /* CRC16 value of ESPNOW data.                                                       */
    uint32_t  magic;                          /* Magic number which is used to determine which device to send unicast ESPNOW data. */
    uint8_t   payload[0];                     /* Real payload of ESPNOW data.                                                      */
} __attribute__((packed)) ESPNOW_DATA;


typedef struct {                              /* Parameters of sending ESPNOW data.                                                */
    bool      unicast;                        /* Send unicast ESPNOW data.                                                         */
    bool      broadcast;                      /* Send broadcast ESPNOW data.                                                       */
    uint8_t   state;                          /* Indicate that if has received broadcast ESPNOW data or not.                       */
    uint32_t  magic;                          /* Magic number which is used to determine which device to send unicast ESPNOW data. */
    uint16_t  count;                          /* Total count of unicast ESPNOW data to be sent.                                    */
    uint16_t  delay;                          /* Delay between sending two ESPNOW data, unit: ms.                                  */
    int       len;                            /* Length of ESPNOW data to be sent, unit: byte.                                     */
    uint8_t  *buffer;                         /* Buffer pointing to ESPNOW data.                                                   */
    uint8_t   dest_mac[ESP_NOW_ETH_ALEN];     /* MAC address of destination device.                                                */
} ESPNOW_SEND_PARAM;

 /*
 *********************************************************************************************************
 *                                           GLOBAL VARIABLES
 *********************************************************************************************************
 */

 /*
 *********************************************************************************************************
 *                                      GLOBAL FUNCTION PROTOTYPES
 *********************************************************************************************************
 */

ESPNOW_EXT esp_err_t  EspNow_Init(void);



#ifdef __cplusplus
}
#endif


#endif /* ESPNOW_H_INCLUDED */
