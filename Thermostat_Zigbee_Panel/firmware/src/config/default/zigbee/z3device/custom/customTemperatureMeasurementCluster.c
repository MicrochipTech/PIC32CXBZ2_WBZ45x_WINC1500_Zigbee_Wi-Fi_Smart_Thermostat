/*******************************************************************************
  Custom Temperature Measurement Cluster Source file

  Company:
    Microchip Technology Inc.

  File Name:
    customTemperatureMeasurementCluster.c

  Summary:
    This file contains Custom Temperature Measurement Cluster Implementation.

  Description:
    This file contains Custom Temperature Measurement Cluster Implementation.
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

#if (APP_Z3_DEVICE_TYPE == APP_DEVICE_TYPE_CUSTOM_DEVICE)

/******************************************************************************
                    Includes section
******************************************************************************/
#include <z3device/custom/include/customTemperatureMeasurementCluster.h>
#include <pds/include/wlPdsMemIds.h>
#include <z3device/common/include/z3Device.h>
#include <app_zigbee/zigbee_console/console.h>
#include <zcl/include/zclCommandManager.h>

/******************************************************************************
                    Definition(s) section
******************************************************************************/

/******************************************************************************
                    Global variables
******************************************************************************/

ZCL_TemperatureMeasurementClusterClientAttributes_t customTemperatureMeasurementClusterClientAttributes =
{
  ZCL_DEFINE_TEMPERATURE_MEASUREMENT_CLUSTER_CLIENT_ATTRIBUTES()
};

/******************************************************************************
                    Local variables section
******************************************************************************/

/******************************************************************************
                    Prototypes section
******************************************************************************/
static void customTemperatureMeasurementReportInd(ZCL_Addressing_t *addressing, uint8_t reportLength, uint8_t *reportPayload);

/******************************************************************************
                    Implementation section
******************************************************************************/
/**************************************************************************//**
\brief Initializes Temperature Measurement cluster
******************************************************************************/
void customTemperatureMeasurementClusterInit(void)
{
  ZCL_Cluster_t *tempcluster = ZCL_GetCluster(APP_ENDPOINT_CUSTOM, TEMPERATURE_MEASUREMENT_CLUSTER_ID, ZCL_CLUSTER_SIDE_CLIENT);
  customTemperatureMeasurementClusterClientAttributes.clusterVersion.value = TEMPERATURE_MEASUREMENT_CLUSTER_VERSION;
  if (tempcluster)
  {
    tempcluster->ZCL_ReportInd = customTemperatureMeasurementReportInd;
  }
  tempcluster->ZCL_DefaultRespInd = ZCL_CommandZclDefaultResp;
}

/**************************************************************************//**
\brief Report attribute indication handler

\param[in] addressing - pointer to addressing information;
\param[in] reportLength - data payload length;
\param[in] reportPayload - data pointer
******************************************************************************/
static void customTemperatureMeasurementReportInd(ZCL_Addressing_t *addressing, uint8_t reportLength, uint8_t *reportPayload)
{
  APP_Zigbee_Event_t event;
  event.eventGroup = EVENT_CLUSTER;
  event.eventId = CMD_ZCL_REPORTING_TEMPERATURE_MEASUREMENT;
  event.eventData.zclEventData.addressing = addressing;
  event.eventData.zclEventData.payloadLength = reportLength;
  event.eventData.zclEventData.payload = reportPayload;

  APP_Zigbee_Handler(event);
}

#endif
// eof customTemperatureMeasurementCluster.c
