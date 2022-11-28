/*******************************************************************************
  MPLAB Harmony Application Header File

  Company:
    Microchip Technology Inc.

  File Name:
    app_mqtt.h

  Summary:
    This header file provides prototypes and definitions for the MQTT application.

  Description:
    This header file provides function prototypes and data type definitions for
    the application.  Some of these are required by the system (such as the
    "APP_Initialize" and "APP_Tasks" prototypes) and some of them are only used
    internally by the application (such as the "APP_STATES" definition).  Both
    are defined here for convenience.
*******************************************************************************/
// DOM-IGNORE-BEGIN
 /*******************************************************************************
* Copyright (C) 2020-21 Microchip Technology Inc. and its subsidiaries.
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


#ifdef APP_MQTT
#include "driver/driver.h"

#define MQTTPacket_willOptions_initializer { {'M', 'Q', 'T', 'W'}, 0, {NULL, {0, NULL}}, {NULL, {0, NULL}}, 0, 0 }

#define MQTTPacket_connectData_initializer { {'M', 'Q', 'T', 'C'}, 0, 4, {NULL, {0, NULL}}, 60, 1, 0, \
		MQTTPacket_willOptions_initializer, {NULL, {0, NULL}}, {NULL, {0, NULL}} }


#define APP_MQTT_BROKER_NAME            "broker.hivemq.com"
#define APP_MQTT_BROKER_PORT            1883

#define APP_MQTT_MAX_TX_BUFF_LEN        512
#define APP_MQTT_MAX_RX_BUFF_LEN        512

#define APP_MQTT_SUBSCRIBE_TOPIC_NAME   "MCHP/WSG/GATEWAY_SUBSCRIBE"
#define APP_MQTT_SUBSCRIBE_QOS          1

#define APP_MQTT_PUBLISH_TOPIC_NAME     "MCHP/WSG/THERMOSTAT_GATEWAY"
#define APP_MQTT_PUBLISH_RETAIN         0
#define APP_MQTT_PUBLISH_QOS            1


#ifdef	__cplusplus
extern "C" {
#endif

typedef enum {
    SOCKETMODE_SOCK_STATE_INIT = 0,
    SOCKETMODE_SOCK_STATE_RESOLVE_IP,
    SOCKETMODE_SOCK_STATE_WAITING_FOR_DNS_RESOLVE,
    SOCKETMODE_SOCK_STATE_CONNECT,
    SOCKETMODE_SOCK_STATE_CONNECTTING,
    SOCKETMODE_SOCK_STATE_CONNECTED,
    SOCKETMODE_SOCK_STATE_ERROR,
} SOCKETMODE_SOCK_STATES;


typedef enum
{
    APP_WIFI_CONNECT ,
    APP_WIFI_DISCONNECT,
    APP_WIFI_PROVISION,
}APP_CALLBACK_NOTIFY;

typedef void (*APP_CALLBACK) (APP_CALLBACK_NOTIFY value,void *data);


extern void APP_MQTT_Initialize(APP_CALLBACK pfAPPWiFiCallback);
extern void APP_MQTT_Tasks(DRV_HANDLE wdrvHandle);
extern void APP_MQTT_Uart_SendMsg(char *message, uint16_t message_len);

#ifdef	__cplusplus
}
#endif

#endif	/* APP_MQTT */

