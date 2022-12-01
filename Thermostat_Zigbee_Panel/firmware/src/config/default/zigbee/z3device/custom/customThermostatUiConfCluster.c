/*******************************************************************************
  Custom Thermostat UIConf Cluster Source file

  Company:
    Microchip Technology Inc.

  File Name:
    customThermostatUiConfCluster.c

  Summary:
    This file contains Custom Thermostat UIConf Cluster Implementation.

  Description:
    This file contains Custom Thermostat UIConf Cluster Implementation.
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
#include <z3device/custom/include/customThermostatUiConfCluster.h>
#include <z3device/custom/include/customThermostatCluster.h>
#include <zcl/include/zclCommandManager.h>
#include <app_zigbee/zigbee_console/console.h>
#include <z3device/common/include/z3Device.h>
#include <app_zigbee/app_zigbee_handler.h>
/*******************************************************************************
                             Defines section
*******************************************************************************/
/*******************************************************************************
                    Static functions prototypes section
*******************************************************************************/

/******************************************************************************
                    Global variables
******************************************************************************/

ZCL_ThermostatUiConfClusterClientAttributes_t customThermostatUiConfClusterClientAttributes =
{  
  ZCL_DEFINE_THERMOSTAT_UI_CONF_CLUSTER_CLIENT_ATTRIBUTES()
};

/******************************************************************************
                    Implementation section
******************************************************************************/
/**************************************************************************//**
\brief Initializes thermostat ui conf cluster
******************************************************************************/
void customThermostatUiConfClusterInit(void)
{
  ZCL_Cluster_t *clusterclient = ZCL_GetCluster(APP_ENDPOINT_CUSTOM, THERMOSTAT_UI_CONF_CLUSTER_ID, ZCL_CLUSTER_SIDE_CLIENT);

  if (clusterclient)
  {
	clusterclient->ZCL_DefaultRespInd = ZCL_CommandZclDefaultResp;
  }

  customThermostatUiConfClusterClientAttributes.clusterVersion.value = THERMOSTAT_UI_CONF_CLUSTER_VERSION;

}


#endif // APP_DEVICE_TYPE_CUSTOM_DEVICE
// eof customThermostatUiConfCluster.c
