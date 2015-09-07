/*****************************************************************************
 *
 * MODULE:             JN-AN-1171
 *
 * COMPONENT:          .h
 *
 * AUTHOR:
 *
 * DESCRIPTION:        ZigBee Light Link Demo Application
 *
 * $HeadURL: https://www.collabnet.nxp.com/svn/lprf_sware/Projects/Zigbee%20Protocol%20Stack/ZPS/Trunk/ZigbeeNodeControlBridge/Source/ZigbeeNodeControlBridge/PDM_IDs.h $
 *
 * $Revision: 54460 $
 *
 * $LastChangedBy: nxp29741 $
 *
 * $LastChangedDate: 2013-06-03 11:39:17 +0100 (Mon, 03 Jun 2013) $
 *
 * $Id: PDM_IDs.h 54460 2013-06-03 10:39:17Z nxp29741 $
 *
 ****************************************************************************
 *
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on NXP products [NXP Microcontrollers such as JN5168, JN5164,
 * JN5161, JN5148, JN5142, JN5139].
 * You, and any third parties must reproduce the copyright and warranty notice
 * and any other legend of ownership on each copy or partial copy of the
 * software.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright NXP B.V. 2012. All rights reserved
 *
 ***************************************************************************/

#ifndef  PDMIDS_H_INCLUDED
#define  PDMIDS_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif


/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/

#include <jendefs.h>

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#ifdef PDM_USER_SUPPLIED_ID

#define PDM_ID_APP_CONTROL_BRIDGE   0x1
#define PDM_ID_APP_ZLL_CMSSION      0x2
#define PDM_ID_APP_END_P_TABLE      0x3
#define PDM_ID_APP_GROUP_TABLE      0x4
#define PDM_ID_APP_APP_ROUTER       0x5
#define PDM_ID_APP_ZLL_ROUTER       0x6
#define PDM_ID_APP_SCENES_CL        0x7
#define PDM_ID_APP_SCENES_ATTB      0x8
#define PDM_ID_APP_SCENES_DATA      0x9

#else

#define PDM_ID_APP_CONTROL_BRIDGE   "CONTROL_BRIDGE"
#define PDM_ID_APP_ZLL_CMSSION      "ZLL_CMSSION"
#define PDM_ID_APP_END_P_TABLE      "END_P_TABLE"
#define PDM_ID_APP_GROUP_TABLE      "GROUP_TABLE"
#define PDM_ID_APP_APP_ROUTER       "APP_ROUTER"
#define PDM_ID_APP_ZLL_ROUTER       "ZLL_ROUTER"
#define PDM_ID_APP_SCENES_CL        "SCENES_CL"
#define PDM_ID_APP_SCENES_ATTB      "SCENES_ATTB"
#define PDM_ID_APP_SCENES_DATA      "SCENES_DATA"

#endif


/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif /* PDMIDS_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
