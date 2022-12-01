/*******************************************************************************
Copyright (C) 2020 released Microchip Technology Inc.  All rights reserved.

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

#include "string.h"
#include <stdlib.h>
#include "definitions.h"
//#include "../../../../config/sam_d21_xpro_winc1500/driver/winc/drv/host_drv/include/winc.h"
#include "MCHP_winc.h"
#include "wdrv_winc_client_api.h"

#define MQTT_RX_POOL_SIZE		512

#define MilliTimer (uint32_t) ((SYS_TIME_Counter64Get() * 1000) / SYS_TIME_FrequencyGet())
int32_t gi32MQTTBrokerRxLen = 0;
bool gbMQTTBrokerRecvDone = false;
bool gbMQTTBrokerSendDone = false;
unsigned char gcMQTTRxFIFO[MQTT_RX_POOL_SIZE];
static uint32_t gu32MQTTRxFIFOPtr = 0;
static uint32_t gu32MQTTRxFIFOLen = 0;

char TimerIsExpired(Timer* timer) {
    if (timer == NULL) {
        return 0;
    }

    long left = timer->end_time - MilliTimer;
    return (left < 0);
}

void TimerCountdownMS(Timer* timer, unsigned int timeout) {
    timer->end_time = MilliTimer + timeout;
}

void TimerCountdown(Timer* timer, unsigned int timeout) {
    timer->end_time = MilliTimer + (timeout * 1000);
}

int TimerLeftMS(Timer* timer) {
    if (timer == NULL) {
        return 0;
    }

    long left = timer->end_time - MilliTimer;
    return (left < 0) ? 0 : left;
}

void TimerInit(Timer* timer) {
    timer->end_time = 0;
}

extern int8_t WINC_SOCKET_RECV(SOCKET sock, void *pvRecvBuf, uint16_t u16BufLen, uint32_t u32Timeoutmsec);
extern int8_t WINC_SOCKET_SEND(SOCKET sock, void *pvSendBuffer, uint16_t u16SendLength, uint16_t u16Flags);
extern int8_t WINC_SOCKET_CLOSE(SOCKET sock);

int WINC_read(Network* n, unsigned char* buffer, int len, int timeout_ms) {
    //at times the upper layer requests for single byte of data, but network has more in rx buffer.
    //this results in callback being invoked multiple times with length 1 before returning. To prevent loss of
    //data in this process, pool data internally and give it to upper layer on request. 

    //temporary workaround for timer overrun 
    if (0 == timeout_ms) timeout_ms = 10;

//    SYS_CONSOLE_PRINT("WINC_read(): Recvd Data (%d B) while len = %d\r\n", gi32MQTTBrokerRxLen, len);
    
    if (0 == gu32MQTTRxFIFOLen) { //no data in internal FIFO

        if((gbMQTTBrokerRecvDone == false) || (gi32MQTTBrokerRxLen == 0))
            return -1;
        
        //	  SYS_CONSOLE_PRINT("DEBUG >> Requesting data from network\r\n");
        gbMQTTBrokerRecvDone = false;

        //update current FIFO length
        if (gi32MQTTBrokerRxLen > 0) { //data recieved form network
            gu32MQTTRxFIFOLen = gi32MQTTBrokerRxLen;
            gu32MQTTRxFIFOPtr = 0;
            //	SYS_CONSOLE_PRINT("DEBUG >> filled FiFo from network. length=%lu\r\n",gu32MQTTRxFIFOLen);
        } else {//mostly in case of timeout
            gu32MQTTRxFIFOLen = 0;
            gu32MQTTRxFIFOPtr = 0;
            //	  SYS_CONSOLE_PRINT("DEBUG >> no data to send. returning error code (%lu)\r\n",gi32MQTTBrokerRxLen);
            return gi32MQTTBrokerRxLen; //this corresponds to the error code.
        }
    }
    //return data to client from data present in the FIFO from previous recv()
    if (len > (int) gu32MQTTRxFIFOLen) {
        SYS_CONSOLE_PRINT("ERROR >> unable to process read request since FIFO contains less data than requested\r\n");
        return -1;
    }

    memcpy((void*) buffer, (const void*) &gcMQTTRxFIFO[gu32MQTTRxFIFOPtr], len);
    gu32MQTTRxFIFOLen -= len;
    gu32MQTTRxFIFOPtr += len;

    return len;
}

int WINC_write(Network* n, unsigned char* buffer, int len, int timeout_ms) {
    gbMQTTBrokerSendDone = false;

    if (SOCK_ERR_NO_ERROR != send(n->socket, buffer, len, 0)) {
        SYS_CONSOLE_PRINT("send() Failed\r\n");
        return -1;
    }

    return len;
}

void WINC_disconnect(Network* n) {
    shutdown(n->socket);
    n->socket = -1;
}

void NetworkInit(Network* n) {
    n->mqttread = WINC_read;
    n->mqttwrite = WINC_write;
    n->disconnect = WINC_disconnect;
}
