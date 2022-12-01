/*******************************************************************************
  Application WINC1500 Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app_winc.c

  Summary:
    This file contains the Application Zigbee implementation for this project.

  Description:
    This file contains the Application Zigbee implementation for this project.
 *******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2022 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*******************************************************************************/
// DOM-IGNORE-END
#include <string.h>
#include "app.h"
#include "app_winc/app_mqtt.h"
#include "definitions.h"
#include "app_winc.h"
#include "app_timer/app_timer.h"
#include "wdrv_winc_client_api.h"
#include "zcl/include/zclThermostatCluster.h"
#include <z3device/thermostat/include/thThermostatCluster.h>

//#define WLAN_SSID           "alexa_iot"
//#define WLAN_AUTH_WPA_PSK
////#define WLAN_AUTH_OPEN
//#define WLAN_PSK            "Microchip1234"

#define WLAN_SSID           "MicrochipDemoAp"
#define WLAN_AUTH_WPA_PSK
//#define WLAN_AUTH_OPEN
#define WLAN_PSK            "12345678"


char mqttPubBuffer[100];


APP_WINC_T winc_state;
bool wifiConnected = false;
static DRV_HANDLE wdrvHandle;

#ifdef APP_MQTT
void APP_Callback(APP_CALLBACK_NOTIFY value,void *data)
{
    
}
#endif

void HVAC_Control(void)
{
    if(thThermostatClusterServerAttributes.localTemperature.value > thThermostatClusterServerAttributes.occupiedCoolingSetpoint.value)
    {
        printf("AC On\r\n");
        RED_LED_Set();
    }
    else
    {
        printf("AC Off\r\n");
        RED_LED_Clear();
    }
}
static void WINC_DHCPAddressEventCallback(DRV_HANDLE handle, uint32_t ipAddress)
{
    char s[20];
    inet_ntop(AF_INET, &ipAddress, s, sizeof(s));
    printf("IP Address: %s\r\n",s);
}

void WINC_MQTT_Publish_ThermostatData(void)
{
//    printf("Publish MQTT Data\r\n");
    
    if ((APP_WINC_MQTT == winc_state) && (wifiConnected == true))
    {
        APP_Msg_T   appMsg;
        appMsg.msgId = APP_MSG_WDRV_EVT;
        appMsg.msgData[0] = APP_WINC_MQTT_PUBLISH;
        OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
        APP_TIMER_SetTimer(APP_TIMER_ID_1, APP_TIMER_30S, false);
    }
}

void WINC_IntiateConnect(void)
{
    APP_Msg_T   appMsg;
    appMsg.msgId = APP_MSG_WDRV_EVT;
    appMsg.msgData[0] = APP_WINC_CONNECT;
    OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
}

static void WINC_ConnectNotifyCallback(DRV_HANDLE handle, WDRV_WINC_ASSOC_HANDLE assocHandle, WDRV_WINC_CONN_STATE currentState, WDRV_WINC_CONN_ERROR errorCode)
{
    APP_Msg_T   appMsg;
    if (WDRV_WINC_CONN_STATE_CONNECTED == currentState)
    {
        wifiConnected = true;
        printf("Connected\r\n");
        appMsg.msgId = APP_MSG_WDRV_EVT;
        appMsg.msgData[0] = APP_WINC_CONNECTED;
        OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
    }
    else if (WDRV_WINC_CONN_STATE_DISCONNECTED == currentState)
    {
        wifiConnected = false;
        if (APP_WINC_CONNECTING == winc_state)
        {
            printf("Failed to connect\r\n");   
        }
        else
        {
            winc_state = APP_WINC_CONNECTING;
            printf("Reconnecting\r\n");
            APP_TIMER_StopTimer(APP_TIMER_ID_1);
            //WINC_IntiateConnect();
        }
        APP_TIMER_SetTimer(APP_TIMER_ID_0, APP_TIMER_30S, false);
    }
    
}

void APP_WINC_Handler(uint8_t *msgData)
{
    APP_Msg_T   appMsg;
    APP_WINC_T  winc_event = (APP_WINC_T)msgData[0];
    switch ( winc_event )
    {
        case APP_WINC_INIT:
        {
            wdrvHandle = WDRV_WINC_Open(0, 0);
#ifdef APP_MQTT
            APP_MQTT_Initialize(&APP_Callback);
#endif
            if (DRV_HANDLE_INVALID != wdrvHandle)
            {
                appMsg.msgId = APP_MSG_WDRV_EVT;
                appMsg.msgData[0] = APP_WINC_CONNECT;
                OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
            }
            else
            {
                printf("Re-Opening WINC\r\n");
                appMsg.msgId = APP_MSG_WDRV_EVT;
                winc_state = APP_WINC_INIT;
                appMsg.msgData[0] = winc_state;
                OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
            }
        }
        break;
        case APP_WINC_CONNECT:
        {
            WDRV_WINC_AUTH_CONTEXT authCtx;
            WDRV_WINC_BSS_CONTEXT  bssCtx;
            
            /* Enable use of DHCP for network configuration, DHCP is the default
             but this also registers the callback for notifications. */

            WDRV_WINC_IPUseDHCPSet(wdrvHandle, &WINC_DHCPAddressEventCallback);
            
            /* Initialize the BSS context to use default values. */

            if (WDRV_WINC_STATUS_OK != WDRV_WINC_BSSCtxSetDefaults(&bssCtx))
            {
                break;
            }
            /* Prepare the BSS context with desired AP's parameters */
            WDRV_WINC_BSSCtxSetChannel(&bssCtx, WLAN_CHANNEL);
            /* Update BSS context with target SSID for connection. */

            if (WDRV_WINC_STATUS_OK != WDRV_WINC_BSSCtxSetSSID(&bssCtx, (uint8_t*)WLAN_SSID, strlen(WLAN_SSID)))
            {
                break;
            }
            
            /* Reset the internal Auth context */
            WDRV_WINC_AuthCtxSetDefaults(&authCtx);
           
            /* Initialize the authentication context for WPA. */

            if (WDRV_WINC_STATUS_OK != WDRV_WINC_AuthCtxSetWPA(&authCtx, (uint8_t*)WLAN_PSK, strlen(WLAN_PSK)))
            {
                break;
            }

            /* Connect to the target BSS with the chosen authentication. */

            if (WDRV_WINC_STATUS_OK == WDRV_WINC_BSSConnect(wdrvHandle, &bssCtx, &authCtx, &WINC_ConnectNotifyCallback))
            {
                printf("Connecting..\r\n");
                appMsg.msgId = APP_MSG_WDRV_EVT;
                winc_state = APP_WINC_CONNECTING;
                appMsg.msgData[0] = winc_state;
                OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
            }
        }
        break;
        case APP_WINC_CONNECTING:
        {
            winc_state = APP_WINC_CONNECTING;
        }
        break;
        case APP_WINC_CONNECTED:
        {
            APP_TIMER_SetTimer(APP_TIMER_ID_1, APP_TIMER_15S, false);
            winc_state = APP_WINC_MQTT;
            appMsg.msgId = APP_MSG_WDRV_EVT;
            appMsg.msgData[0] = winc_state;
            OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
        }
        break;
        case APP_WINC_MQTT:
        {
            APP_MQTT_Tasks(wdrvHandle);
            //winc_state = APP_WINC_MQTT;
            appMsg.msgId = APP_MSG_WDRV_EVT;
            appMsg.msgData[0] = APP_WINC_MQTT;
            OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
        }
        break;
        case APP_WINC_MQTT_PUBLISH:
        {
            if( winc_state == APP_WINC_MQTT)
            {
//                 sprintf(mqttPubBuffer, "{\"Current temperature\" : %d, \n}",(int)thThermostatClusterServerAttributes.localTemperature.value);
                sprintf(mqttPubBuffer, "{\"Current Temp\": %d.%d,\n\"Set Temp\": %d.%d,\n\"Air Conditioner\": \"%s\"}", 
                        (int)thThermostatClusterServerAttributes.localTemperature.value/ZCL_THERMOSTAT_TEMPERATURE_SCALE,
                        (int)thThermostatClusterServerAttributes.localTemperature.value%ZCL_THERMOSTAT_TEMPERATURE_SCALE,
                        (int)thThermostatClusterServerAttributes.occupiedCoolingSetpoint.value/ZCL_THERMOSTAT_TEMPERATURE_SCALE,
                        (int)thThermostatClusterServerAttributes.occupiedCoolingSetpoint.value%ZCL_THERMOSTAT_TEMPERATURE_SCALE,
                        thThermostatClusterServerAttributes.localTemperature.value > thThermostatClusterServerAttributes.occupiedCoolingSetpoint.value ? "On\0" : "Off\0");
                printf(mqttPubBuffer);
                if(wifiConnected)
                {
                    APP_MQTT_Uart_SendMsg(mqttPubBuffer, strlen(mqttPubBuffer));
                }
            }
            HVAC_Control();
        }
        break;
        default:
        {
            
        }
        break;
    }
}