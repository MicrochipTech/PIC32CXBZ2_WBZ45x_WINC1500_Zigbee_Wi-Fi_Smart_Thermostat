/*******************************************************************************
  MPLAB Harmony Application Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app_socket.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It
    implements the logic of the application's state machine and it may call
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

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

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************


#include "wdrv_winc_client_api.h"
#include "app_mqtt.h"

#define SOCK_HOST_NAME              APP_MQTT_BROKER_NAME        //"broker.hivemq.com"
#define SOCK_HOST_PORT              APP_MQTT_BROKER_PORT        //1883 
#define SOCK_TCP_RECV_BUFFER_SIZE   1460
#define SOCK_SERVER_NAME            64

// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************

typedef struct {
    uint32_t hostIp;
    SOCKETMODE_SOCK_STATES sock_state;
    uint16_t hostPort;
    char hostName[SOCK_SERVER_NAME];
    uint8_t recvBuffer[SOCK_TCP_RECV_BUFFER_SIZE];
} SOCKETMODE_SOCK_Config;

static SOCKETMODE_SOCK_Config g_sSockConfig;
SOCKET g_clientSocket = -1;
uint32_t g_u32SockStatus = 0;
extern bool gbMQTTBrokerSendDone;
extern bool gbMQTTBrokerRecvDone;

#define MQTT_RX_POOL_SIZE		512
extern unsigned char gcMQTTRxFIFO[MQTT_RX_POOL_SIZE];
extern int32_t gi32MQTTBrokerRxLen;

// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    This structure should be initialized by the APP_Initialize function.

    Application strings and buffers are be defined outside this structure.
 */

// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************

static void SocketMode_ExampleSocketResolverCallback(uint8_t *pu8DomainName, uint32_t u32IP) {
    char s[20];

    SYS_CONSOLE_PRINT("%s: IP address is %s\r\n", pu8DomainName, inet_ntop(AF_INET, &u32IP, s, sizeof (s)));

    g_sSockConfig.hostIp = u32IP;

    g_sSockConfig.sock_state = SOCKETMODE_SOCK_STATE_CONNECT;
}

static int8_t SocketMode_ExampleSocketConnect(void) {
    struct sockaddr_in addr_in;

    addr_in.sin_family = AF_INET;
    addr_in.sin_port = _htons(SOCK_HOST_PORT);
    addr_in.sin_addr.s_addr = g_sSockConfig.hostIp;

    /* Create secure socket */
    if (g_clientSocket < 0) {
        g_clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    }

    /* Check if socket was created successfully */
    if (g_clientSocket == -1) {
        SYS_CONSOLE_PRINT("socket error.\r\n");
        shutdown(g_clientSocket);
        return -1;
    }

    /* If success, connect to socket */
    if (connect(g_clientSocket, (struct sockaddr *) &addr_in, sizeof (struct sockaddr_in)) != SOCK_ERR_NO_ERROR) {
        SYS_CONSOLE_PRINT("connect error.\r\n");
        return SOCK_ERR_INVALID;
    }

    /* Success */
    return SOCK_ERR_NO_ERROR;
}

static void SocketMode_ExampleSslClientEventCallback(SOCKET socket, uint8_t messageType, void *pMessage) {
    switch (messageType) {
        case SOCKET_MSG_RECV:
        {
            tstrSocketRecvMsg *pRecvMessage = (tstrSocketRecvMsg*) pMessage;
            
            gbMQTTBrokerRecvDone = true;
            
            gi32MQTTBrokerRxLen = 0;
            
            if ((NULL != pRecvMessage) && (pRecvMessage->s16BufferSize > 0)) {
                gi32MQTTBrokerRxLen = pRecvMessage->s16BufferSize;
            }
            
            while(SOCK_ERR_NO_ERROR != recv(socket, gcMQTTRxFIFO, MQTT_RX_POOL_SIZE, 0));
            
            break;
        }

        case SOCKET_MSG_SEND:
        {
            gbMQTTBrokerSendDone = true;
            break;
        }

        case SOCKET_MSG_CONNECT:
        {
            g_sSockConfig.sock_state = SOCKETMODE_SOCK_STATE_CONNECTED;
            
            g_u32SockStatus = 1;

            while(SOCK_ERR_NO_ERROR != recv(socket, gcMQTTRxFIFO, MQTT_RX_POOL_SIZE, 0));
            
            break;
        }

        default:
        {
            break;
        }
    }
}

void APP_Sock_Connect_Initialize(DRV_HANDLE wdrvHandle) {
    memcpy(g_sSockConfig.hostName, SOCK_HOST_NAME, strlen(SOCK_HOST_NAME));

    g_sSockConfig.hostPort = SOCK_HOST_PORT;

    g_clientSocket = -1;

    g_sSockConfig.sock_state = SOCKETMODE_SOCK_STATE_INIT;
}

// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

void APP_Sock_Connect_Tasks(DRV_HANDLE wdrvHandle) {
    switch (g_sSockConfig.sock_state) {
        case SOCKETMODE_SOCK_STATE_INIT:
        {
            if (false == WDRV_WINC_IPLinkActive(wdrvHandle)) {
                break;
            }

            WDRV_WINC_SocketRegisterResolverCallback(wdrvHandle, &SocketMode_ExampleSocketResolverCallback);

            g_sSockConfig.sock_state = SOCKETMODE_SOCK_STATE_RESOLVE_IP;
        }
            break;

        case SOCKETMODE_SOCK_STATE_RESOLVE_IP:
        {
            if (gethostbyname(g_sSockConfig.hostName) == SOCK_ERR_NO_ERROR) {
                g_sSockConfig.sock_state = SOCKETMODE_SOCK_STATE_WAITING_FOR_DNS_RESOLVE;
            }
        }
            break;

        case SOCKETMODE_SOCK_STATE_WAITING_FOR_DNS_RESOLVE:
        {

        }
            break;

        case SOCKETMODE_SOCK_STATE_CONNECT:
        {
            /* Register callback for socket events. */
            WDRV_WINC_SocketRegisterEventCallback(wdrvHandle, &SocketMode_ExampleSslClientEventCallback);

            SocketMode_ExampleSocketConnect();

            g_sSockConfig.sock_state = SOCKETMODE_SOCK_STATE_CONNECTTING;
        }
            break;

        case SOCKETMODE_SOCK_STATE_CONNECTTING:
        {

        }
            break;

        case SOCKETMODE_SOCK_STATE_CONNECTED:
        {

        }
            break;

        case SOCKETMODE_SOCK_STATE_ERROR:
        {

        }
            break;
    }
}

/*******************************************************************************
 End of File
 */
