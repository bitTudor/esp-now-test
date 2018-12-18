/* ESPNOW Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/*
   This example shows how to use ESPNOW.
   Prepare two device, one for sending ESPNOW data and another for receiving
   ESPNOW data.
*/

#include "espnow.h"


void app_main()
{
    // Initialize NVS
    ESP_ERROR_CHECK( nvs_flash_init() );

    EspNow_WifiInit();
    EspNow_Init();
}
