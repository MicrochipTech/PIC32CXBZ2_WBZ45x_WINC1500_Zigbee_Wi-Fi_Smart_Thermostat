/*******************************************************************************
Copyright (C) 2020-21 released Microchip Technology Inc.  All rights reserved.

Microchip licenses to you the right to use, modify, copy and distribute
Software only when embedded on a Microchip microcontroller or digital signal
controller that is integrated into your product or third party product
(pursuant to the sublicense terms in the accompanying license agreement).

You should refer to the license agreement accompanying this Software for
additional information regarding your rights and obligations.

SOFTWARE AND DOCUMENTATION ARE PROVIDED AS IS WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER
CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR
OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR
CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF
SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
(INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
 *******************************************************************************/

#include "app.h"

#ifdef APP_MQTT

//#define SYS_CONSOLE_PRINT(fmt, ...)     printf(__VA_ARGS__)

#include "configuration.h"
#include "third_party/paho.mqtt.embedded-c/MQTTClient-C/src/MQTTClient.h" 
#include "third_party/paho.mqtt.embedded-c/MQTTClient-C/Platforms/MCHP_winc.h"
#include "nm_common.h"
#include "wdrv_winc_client_api.h"
#include "app_mqtt.h"

typedef enum {
    APP_MQTT_STATUS_IDLE = 0, // Idle
    APP_MQTT_STATUS_LOWER_LAYER_DOWN, // Lower Layer is DOWN
    APP_MQTT_STATUS_SOCK_CLIENT_CONNECTING, // Net Client connecting to Net Server
    APP_MQTT_STATUS_SOCK_CONNECTED, // Net Instance connected to the peer
    APP_MQTT_STATUS_MQTT_CONNECTED, // Lower Layer is DOWN
    APP_MQTT_STATUS_MQTT_DISCONNECTING, // Net Instance in disconnected state
    APP_MQTT_STATUS_MQTT_DISCONNECTED, // Net Instance in disconnected state
    APP_MQTT_STATUS_MQTT_CONN_FAILED, // Lower Layer is DOWN
    APP_MQTT_STATUS_WAIT_FOR_MQTT_CONACK, // Wait for Connect Ack from Broker
    APP_MQTT_STATUS_WAIT_FOR_MQTT_SUBACK, // Wait for Subscribe Ack from Broker
    APP_MQTT_STATUS_WAIT_FOR_MQTT_PUBACK, // Wait for Publish Ack from Broker
} APP_MQTT_STATUS;

#define APP_MQTT_SUBSCRIBE_TOPIC_NAME_MAX_LEN       128
#define APP_MQTT_MAX_BROKER_NAME_LEN                256
#define APP_MQTT_SUB_MAX_TOPICS                     1
#define APP_MQTT_MSG_MAX_LEN                        512
#define APP_MQTT_CLIENT_ID_MAX_LEN                  256

typedef struct {
    char brokerName[APP_MQTT_MAX_BROKER_NAME_LEN]; // MQTT Broker/ Server Name
    uint16_t serverPort; // MQTT Server Port
    uint16_t keepAliveInterval; // Keep Alive Interval for the Mqtt Session
    char clientId[APP_MQTT_CLIENT_ID_MAX_LEN]; // MQTT Client ID
} APP_MQTT_BrokerConfig;

typedef struct {
    uint8_t entryValid;
    uint8_t qos; //Qos (0/ 1/ 2)
    char topicName[APP_MQTT_SUBSCRIBE_TOPIC_NAME_MAX_LEN]; //Name of the Topic Subscribing to
} APP_MQTT_SubscribeConfig;

typedef struct {
    APP_MQTT_BrokerConfig sBrokerConfig; //MQTT Broker Configuration
    uint8_t subscribeCount; //Number of Topis Subscribed to (0-APP_MQTT_MAX_TOPICS)
    APP_MQTT_SubscribeConfig sSubscribeConfig[APP_MQTT_SUB_MAX_TOPICS]; //Config for all the Topics Subscribed to
} APP_MQTT_Config;

#define APP_MQTT_PAHO_MAX_TX_BUFF_LEN  512
#define APP_MQTT_PAHO_MAX_RX_BUFF_LEN  512

typedef struct {
    Network sPahoNetwork;
    MQTTClient sPahoClient;
    unsigned char sendbuf[APP_MQTT_PAHO_MAX_TX_BUFF_LEN];
    unsigned char recvbuf[APP_MQTT_PAHO_MAX_RX_BUFF_LEN];
} APP_MQTT_PahoInfo;

typedef struct {
    APP_MQTT_Config sCfgInfo;
    APP_MQTT_PahoInfo sPahoInfo; /* Info related to - Paho */
    APP_MQTT_STATUS eStatus; /* Current state of the service */
    Timer timerInfo;
} APP_MQTT_Handle;

APP_MQTT_Handle g_sSysMqttHandle;
MQTTMessage g_sMqttMsg;

extern uint32_t g_u32SockStatus;

// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

static inline void APP_MQTT_SetInstStatus(APP_MQTT_Handle *hdl, APP_MQTT_STATUS status) {
    hdl->eStatus = status;
    // SYS_CONSOLE_PRINT("Handle (0x%p) State (%d)\r\n", hdl, status);
}

#define APP_MQTT_TIMEOUT_CONST (5000) //msec

extern void APP_Sock_Connect_Tasks(DRV_HANDLE wdrvHandle);
extern void APP_Sock_Connect_Initialize(DRV_HANDLE wdrvHandle);
extern SOCKET g_clientSocket;

void APP_MQTT_StartTimer(APP_MQTT_Handle *hdl, uint32_t timerInfo) {
    TimerInit(&hdl->timerInfo);
    TimerCountdownMS(&hdl->timerInfo, timerInfo);
}

bool APP_MQTT_TimerExpired(APP_MQTT_Handle *hdl) {
    if (TimerIsExpired(&hdl->timerInfo))
        return true;
    else
        return false;
}

void APP_MQTT_ResetTimer(APP_MQTT_Handle *hdl) {
    TimerInit(&hdl->timerInfo);
}

void APP_MQTT_ProcessTimeout(APP_MQTT_Handle *hdl, APP_MQTT_STATUS nextStatus) {
    APP_MQTT_ResetTimer(hdl);

    APP_MQTT_SetInstStatus(hdl, nextStatus);
}

int32_t APP_MQTT_SendMsg(char *topicName, char *message, uint16_t message_len, uint8_t qos, uint8_t retain) {
    int rc = 0;

    memset(&g_sMqttMsg, 0, sizeof (g_sMqttMsg));

    g_sMqttMsg.dup = 0;

    g_sMqttMsg.id = 1;

    g_sMqttMsg.payload = message;

    g_sMqttMsg.payloadlen = message_len;

    g_sMqttMsg.qos = qos;

    g_sMqttMsg.retained = retain;

    rc = MQTTPublish(&(g_sSysMqttHandle.sPahoInfo.sPahoClient),
            topicName,
            &g_sMqttMsg);
    if (rc != 0) {
        SYS_CONSOLE_PRINT("MQTTPublish() Failed (%d)\r\n", rc);

        return -1;
    }

    APP_MQTT_StartTimer(&g_sSysMqttHandle, APP_MQTT_TIMEOUT_CONST);

    if (qos != 0) {
        APP_MQTT_SetInstStatus(&g_sSysMqttHandle, APP_MQTT_STATUS_WAIT_FOR_MQTT_PUBACK);
    }

    SYS_CONSOLE_PRINT("Published to Topic (%s)\r\n", topicName);

    return 0;
}

/* Callback registered with Paho SW to get the messages received on the subscribed topic */
void APP_MQTT_messageCallback(MessageData* data) {
    char *msg = data->message->payload;

    char topic[32] = {0};

    strncpy(topic, (char *) data->topicName->lenstring.data, data->topicName->lenstring.len);

    topic[data->topicName->lenstring.len] = 0;

    SYS_CONSOLE_PRINT("\nTopic = %s ; Message = %s\r\n", topic, msg);
}

//static int APP_MqttCMDProcess
//(
//    SYS_CMD_DEVICE_NODE* pCmdIO, 
//    int argc, 
//    char** argv
//) 
//{
//   if (argc > 2)
//   {
//       return -1;
//   }
//   
//   if (!strcmp(argv[1], "help"))
//   {
//        SYS_CONSOLE_PRINT("mqtt broker <url>  : Set the broker address\r\n");
//        SYS_CONSOLE_PRINT("mqtt port <port_number> : Set the port number\r\n");
//        SYS_CONSOLE_PRINT("mqtt start : Start the MQTT application\r\n");
//   }
//   else if(!strcmp(argv[1], "start"))
//   {
//       APP_MQTT_SetInstStatus(&g_sSysMqttHandle, APP_MQTT_STATUS_LOWER_LAYER_DOWN); 
//   }
//   return 0;
//}
//
//static const SYS_CMD_DESCRIPTOR AppMqttCmdTbl[] =
//{
//    {"mqtt", (SYS_CMD_FNC) APP_MqttCMDProcess, ": Socket mode MQTT commands processing"},
//    
//};

void APP_MQTT_Initialize(APP_CALLBACK pfAPPWiFiCallback) 
{
    strcpy(g_sSysMqttHandle.sCfgInfo.sBrokerConfig.brokerName, APP_MQTT_BROKER_NAME);

    g_sSysMqttHandle.sCfgInfo.sBrokerConfig.serverPort = APP_MQTT_BROKER_PORT;

    g_sSysMqttHandle.sCfgInfo.subscribeCount = 1;

    strcpy(g_sSysMqttHandle.sCfgInfo.sSubscribeConfig[0].topicName, APP_MQTT_SUBSCRIBE_TOPIC_NAME);

    g_sSysMqttHandle.sCfgInfo.sSubscribeConfig[0].qos = APP_MQTT_SUBSCRIBE_QOS;

//    SYS_CMD_ADDGRP(AppMqttCmdTbl, sizeof (AppMqttCmdTbl) / sizeof (*AppMqttCmdTbl), "mqtt", ": Socket mode MQTT commands"); 
    APP_MQTT_SetInstStatus(&g_sSysMqttHandle, APP_MQTT_STATUS_LOWER_LAYER_DOWN);
    //APP_MQTT_SetInstStatus(&g_sSysMqttHandle, APP_MQTT_STATUS_IDLE);
}

void APP_MQTT_Tasks(DRV_HANDLE wdrvHandle) 
{
    APP_MQTT_Handle *hdl = &g_sSysMqttHandle;

    if ((APP_MQTT_STATUS_LOWER_LAYER_DOWN != hdl->eStatus) && (APP_MQTT_STATUS_IDLE != hdl->eStatus)) {       //sankar
        APP_Sock_Connect_Tasks(wdrvHandle);
    }

    switch (hdl->eStatus) {
            /* Lower Layer is Down */
        case APP_MQTT_STATUS_LOWER_LAYER_DOWN:
        {
            SYS_CONSOLE_PRINT("Host Name = %s \r\n", hdl->sCfgInfo.sBrokerConfig.brokerName);

            APP_Sock_Connect_Initialize(wdrvHandle);

            APP_MQTT_SetInstStatus(hdl, APP_MQTT_STATUS_SOCK_CLIENT_CONNECTING);

        }
            break;

        case APP_MQTT_STATUS_SOCK_CLIENT_CONNECTING:
        {
            /* Poll for status of TCP Socket */
            if (g_u32SockStatus) {
                APP_MQTT_SetInstStatus(hdl, APP_MQTT_STATUS_SOCK_CONNECTED);

                hdl->sPahoInfo.sPahoNetwork.socket = g_clientSocket;
            }
        }
            break;

            /* Socket Connection up */
        case APP_MQTT_STATUS_SOCK_CONNECTED:
        {
            int rc = 0;

            char buffer[80];

            memset(buffer, 0, sizeof (buffer));

            /* Open the MQTT Connection */
            NetworkInit(&(hdl->sPahoInfo.sPahoNetwork));

            MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;

            MQTTClientInit(&(hdl->sPahoInfo.sPahoClient),
                    &(hdl->sPahoInfo.sPahoNetwork),
                    60000,
                    hdl->sPahoInfo.sendbuf,
                    APP_MQTT_MAX_TX_BUFF_LEN,
                    hdl->sPahoInfo.recvbuf,
                    APP_MQTT_MAX_RX_BUFF_LEN);

            connectData.MQTTVersion = 4; //use protocol version 3.1.1

            if (strlen(hdl->sCfgInfo.sBrokerConfig.clientId) == 0) {
                uint8_t au8MacAddr[6] = {0};

                if (m2m_wifi_get_mac_address(au8MacAddr) == M2M_SUCCESS) {
                    sprintf(hdl->sCfgInfo.sBrokerConfig.clientId, "MCHP_%.2x%.2x%.2x",
                            *(au8MacAddr + 3), *(au8MacAddr + 4), *(au8MacAddr + 5));

                    hdl->sCfgInfo.sBrokerConfig.clientId[11] = 0;
                } else {
                    strcpy(hdl->sCfgInfo.sBrokerConfig.clientId, "mchp");

                    hdl->sCfgInfo.sBrokerConfig.clientId[strlen("mchp")] = 0;
                }
            }

            connectData.clientID.cstring = (char *) &(hdl->sCfgInfo.sBrokerConfig.clientId);

            connectData.keepAliveInterval = hdl->sCfgInfo.sBrokerConfig.keepAliveInterval;

            /*
             ** In case we have username/ password scenario
            {
                connectData.username.cstring = ?

                connectData.password.cstring = ?
            }
             */

            /*
             ** In case we have to configure LWT
            {
                connectData.willFlag = 1;

                connectData.will.message.cstring = ?;

                connectData.will.topicName.cstring = ?;

                connectData.will.qos = ?

                connectData.will.retained = ?;
            }
             */

            if ((rc = MQTTConnect(&(hdl->sPahoInfo.sPahoClient), &connectData)) != 0) {
                SYS_CONSOLE_PRINT("MQTTConnect() failed (%d)\r\n", rc);

                APP_MQTT_SetInstStatus(hdl, APP_MQTT_STATUS_MQTT_CONN_FAILED);

                return;
            }

            APP_MQTT_StartTimer(hdl, APP_MQTT_TIMEOUT_CONST);

            APP_MQTT_SetInstStatus(hdl, APP_MQTT_STATUS_WAIT_FOR_MQTT_CONACK);
        }
            break;

            /* Waiting for Connection Ack */
        case APP_MQTT_STATUS_WAIT_FOR_MQTT_CONACK:
        {
            /* Wait for the MQTT Connection Ack from the MQTT Server */
            int rc = MQTTWaitForConnect(&(hdl->sPahoInfo.sPahoClient));
            if (rc == SUCCESS) {
                APP_MQTT_ResetTimer(hdl);

                APP_MQTT_SetInstStatus(hdl, APP_MQTT_STATUS_MQTT_CONNECTED);

                SYS_CONSOLE_PRINT("MQTT Connected\r\n\n");

                /* Check if the Application configured a Topic to 
                 * Subscribe to while opening the MQTT Service */
                if (hdl->sCfgInfo.subscribeCount) {
                    SYS_CONSOLE_PRINT("Subscribing to Topic = %s\r\n", hdl->sCfgInfo.sSubscribeConfig[0].topicName);

                    if ((rc = MQTTSubscribe(&(hdl->sPahoInfo.sPahoClient),
                            hdl->sCfgInfo.sSubscribeConfig[0].topicName,
                            (enum QoS)(hdl->sCfgInfo.sSubscribeConfig[0].qos),
                            APP_MQTT_messageCallback)) != 0) {
                        SYS_CONSOLE_PRINT("MQTTSubscribe() failed (%d)\r\n", rc);

                        return;
                    }

                    APP_MQTT_StartTimer(hdl, APP_MQTT_TIMEOUT_CONST);

                    APP_MQTT_SetInstStatus(hdl, APP_MQTT_STATUS_WAIT_FOR_MQTT_SUBACK);
                }
            } else {
                if(APP_MQTT_TimerExpired(hdl) == true)
                    APP_MQTT_ProcessTimeout(hdl, APP_MQTT_STATUS_MQTT_CONN_FAILED);
            }
        }
            break;

            /* Waiting for Sub Ack */
        case APP_MQTT_STATUS_WAIT_FOR_MQTT_SUBACK:
        {
            /* Wait for the MQTT Subscribe Ack */
            int rc = MQTTWaitForSubscribeAck(&(hdl->sPahoInfo.sPahoClient),
                    APP_MQTT_SUBSCRIBE_TOPIC_NAME,
                    APP_MQTT_messageCallback);
            if (rc == SUCCESS) {
                APP_MQTT_SubscribeConfig sMqttSubCfg;

                APP_MQTT_ResetTimer(hdl);

                memset(&sMqttSubCfg, 0, sizeof (sMqttSubCfg));

                strcpy(sMqttSubCfg.topicName, APP_MQTT_SUBSCRIBE_TOPIC_NAME);

                SYS_CONSOLE_PRINT("Suback received for Topic (%s)\r\n\n", APP_MQTT_SUBSCRIBE_TOPIC_NAME);

                APP_MQTT_SetInstStatus(hdl, APP_MQTT_STATUS_MQTT_CONNECTED);

//                APP_MQTT_SendMsg(APP_MQTT_PUBLISH_TOPIC_NAME, "Hello",
//                        strlen("Hello"), APP_MQTT_PUBLISH_QOS,
//                        APP_MQTT_PUBLISH_RETAIN);
            } else {
                if(APP_MQTT_TimerExpired(hdl) == true)
                    APP_MQTT_ProcessTimeout(hdl, APP_MQTT_STATUS_MQTT_CONNECTED);
            }
        }
            break;

        case APP_MQTT_STATUS_WAIT_FOR_MQTT_PUBACK:
        {
            /* Wait for the MQTT Publish Ack */
            int rc = MQTTWaitForPublishAck(&(hdl->sPahoInfo.sPahoClient),
                    &g_sMqttMsg);
            if (rc == SUCCESS) {
                APP_MQTT_ResetTimer(hdl);

                APP_MQTT_SetInstStatus(hdl, APP_MQTT_STATUS_MQTT_CONNECTED);

                SYS_CONSOLE_PRINT("Puback received\r\n\n");
            } else {
                if(APP_MQTT_TimerExpired(hdl) == true)
                    APP_MQTT_ProcessTimeout(hdl, APP_MQTT_STATUS_MQTT_CONNECTED);
            }
        }
            break;

            /* MQTT Connection Up */
        case APP_MQTT_STATUS_MQTT_CONNECTED:
        {
            /* Poll for status of TCP Socket */
            if (g_u32SockStatus == 0) {
                APP_MQTT_SetInstStatus(hdl, APP_MQTT_STATUS_MQTT_DISCONNECTING);
            }

            /* Wait for any message on the Subscribed Topics */
            int rc = MQTTYield(&(hdl->sPahoInfo.sPahoClient), 50);
            if (rc == SUCCESS) {
            }
        }
            break;

            /* MQTT Disconnected */
        case APP_MQTT_STATUS_MQTT_DISCONNECTING:
        {
            APP_MQTT_SetInstStatus(hdl, APP_MQTT_STATUS_MQTT_DISCONNECTED);
        }
            break;

        case APP_MQTT_STATUS_IDLE:
        {
         
            break;
        }
        case APP_MQTT_STATUS_MQTT_DISCONNECTED:
        case APP_MQTT_STATUS_MQTT_CONN_FAILED:
        {
            SYS_CONSOLE_PRINT("Connection Failed\r\n\n");
        }
            break;
    }
}

void APP_MQTT_Uart_SendMsg(char *message, uint16_t message_len)
{
    APP_MQTT_SendMsg(APP_MQTT_PUBLISH_TOPIC_NAME, message,
        message_len, APP_MQTT_PUBLISH_QOS,
            APP_MQTT_PUBLISH_RETAIN);
}

#endif //#ifdef APP_MQTT