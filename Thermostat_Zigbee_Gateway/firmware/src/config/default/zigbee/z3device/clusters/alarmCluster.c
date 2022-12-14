/*******************************************************************************
  Alarm Cluster Source File

  Company:
    Microchip Technology Inc.

  File Name:
    alarmsCluster.c

  Summary:
    This file contains Alarms Cluster Implemenation.

  Description:
    This file contains Alarms Cluster Implemenation.
 *******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2018 Microchip Technology Inc. and its subsidiaries.
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

/******************************************************************************
                    Includes section
******************************************************************************/
#include <z3device/clusters/include/haClusters.h>
#include <zcl/include/zclCommandManager.h>
#include <pds/include/wlPdsMemIds.h>
#include <zcl/include/zclAlarmsCluster.h>
#include <zcl/include/zclThermostatCluster.h>
#if (APP_Z3_DEVICE_TYPE == APP_DEVICE_TYPE_THERMOSTAT)
#include <z3device/thermostat/include/thAlarmsCluster.h>
#endif
/******************************************************************************
                    Macro definitions
******************************************************************************/

/******************************************************************************
                    Prototypes section
******************************************************************************/


/******************************************************************************
                    Global variables section
******************************************************************************/

/******************************************************************************
                    Implementation section
******************************************************************************/

/**************************************************************************//**
\brief Indication of alarm notification response

\param[in] ntfy - pointer to response
******************************************************************************/
static void alarmNotificationCB(ZCL_Notify_t *ntfy)
{
  (void)ntfy;
}

/**************************************************************************//**
\brief alarm notification raised by other clusters

\param[in] ZCL_Alarm_t alarm
******************************************************************************/
void ZCL_AlarmNotification(ZCL_Alarm_t *alarm, Endpoint_t srcEndPoint,bool masked)
{
  if(masked)
  {
    ZCL_Request_t *req;
    // send alarm notification
    if ((req = getFreeCommand()))
    {
      memcpy(req->requestPayload,(uint8_t *)alarm,sizeof(ZCL_Alarm_t));
      req->requestLength = sizeof(ZCL_Alarm_t);
      /*Send Alarm Notification if its not masked*/
      fillCommandRequest(req, ZCL_ALARMS_CLUSTER_CLIENT_ALARM_COMMAND_ID, sizeof(ZCL_Alarm_t), srcEndPoint);
      fillDstAddressing(&req->dstAddressing, APS_NO_ADDRESS, 0x00, srcEndPoint, ALARMS_CLUSTER_ID);
      req->dstAddressing.clusterSide = ZCL_CLUSTER_SIDE_CLIENT;
      req->ZCL_Notify = alarmNotificationCB;
      ZCL_CommandManagerSendCommand(req);
    }
  }
#if (APP_Z3_DEVICE_TYPE == APP_DEVICE_TYPE_THERMOSTAT)
  /*Add an entry whether its masked or otherwise Section 3.11 line 4809 ZCl Rev 7 */
  addAlarmEntry(alarm->alarmCode,alarm->clusterIdentifier);
#endif
}



//eof alarmCluster.c
