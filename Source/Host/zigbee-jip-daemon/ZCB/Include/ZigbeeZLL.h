/****************************************************************************
 *
 * MODULE:             Linux Zigbee - JIP daemon
 *
 * COMPONENT:          Interface to Zigbee ZLL device
 *
 * REVISION:           $Revision: 43420 $
 *
 * DATED:              $Date: 2012-06-18 15:13:17 +0100 (Mon, 18 Jun 2012) $
 *
 * AUTHOR:             Lee Mitchell
 *
 ****************************************************************************
 *
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on NXP products [NXP Microcontrollers such as JN5148, JN5142, JN5139]. 
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

 * Copyright NXP B.V. 2012. All rights reserved
 *
 ***************************************************************************/

#ifndef  ZIGBEEZLL_H_INCLUDED
#define  ZIGBEEZLL_H_INCLUDED

#include <stdint.h>

#include "Utils.h"

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include "ZigbeeControlBridge.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/


/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/


/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

    

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

teZcbStatus eZBZLL_OnOff(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress, uint8_t u8Mode);
teZcbStatus eZBZLL_OnOffCheck(tsZCB_Node *psZCBNode, uint8_t u8Mode);

teZcbStatus eZBZLL_MoveToLevel(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress, uint8_t u8OnOff, uint8_t u8Level, uint16_t u16TransitionTime);
teZcbStatus eZBZLL_MoveToLevelCheck(tsZCB_Node *psZCBNode, uint8_t u8OnOff, uint8_t u8Level, uint16_t u16TransitionTime);

teZcbStatus eZBZLL_MoveToHue(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress, uint8_t u8Hue, uint16_t u16TransitionTime);
teZcbStatus eZBZLL_MoveToSaturation(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress, uint8_t u8Saturation, uint16_t u16TransitionTime);
teZcbStatus eZBZLL_MoveToHueSaturation(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress, uint8_t u8Hue, uint8_t u8Saturation, uint16_t u16TransitionTime);

teZcbStatus eZBZLL_MoveToColour(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress, uint16_t u16X, uint16_t u16Y, uint16_t u16TransitionTime);

teZcbStatus eZBZLL_MoveToColourTemperature(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress, uint16_t u16ColourTemperature, uint16_t u16TransitionTime);
teZcbStatus eZBZLL_MoveColourTemperature(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress, uint8_t u8Mode, uint16_t u16Rate, uint16_t u16ColourTemperatureMin, uint16_t u16ColourTemperatureMax);

teZcbStatus eZBZLL_ColourLoopSet(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress, uint8_t u8UpdateFlags, uint8_t u8Action, uint8_t u8Direction, uint16_t u16Time, uint16_t u16StartHue);

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* ZIGBEEZLL_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

