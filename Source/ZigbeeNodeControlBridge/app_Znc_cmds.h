/*****************************************************************************
 *
 * MODULE: ZigbeeNodeControlBridge
 *
 * COMPONENT: app_Znc_cmds.h
 *
 * $AUTHOR: Faisal Bhaiyat$
 *
 * DESCRIPTION:
 *
 * $HeadURL: https://www.collabnet.nxp.com/svn/lprf_sware/Projects/Zigbee%20Protocol%20Stack/ZPS/Trunk/ZigbeeNodeControlBridge/Source/ZigbeeNodeControlBridge/app_Znc_cmds.h $
 *
 * $Revision: 54776 $
 *
 * $LastChangedBy: nxp29741 $
 *
 * $LastChangedDate: 2013-06-20 11:50:33 +0100 (Thu, 20 Jun 2013) $
 *
 * $Id: app_Znc_cmds.h 54776 2013-06-20 10:50:33Z nxp29741 $
 *
 *****************************************************************************
 *
 * This software is owned by Jennic and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on Jennic products. You, and any third parties must reproduce
 * the copyright and warranty notice and any other legend of ownership on each
 * copy or partial copy of the software.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS". JENNIC MAKES NO WARRANTIES, WHETHER
 * EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
 * ACCURACY OR LACK OF NEGLIGENCE. JENNIC SHALL NOT, IN ANY CIRCUMSTANCES,
 * BE LIABLE FOR ANY DAMAGES, INCLUDING, BUT NOT LIMITED TO, SPECIAL,
 * INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON WHATSOEVER.
 *
 * Copyright Jennic Ltd 2008. All rights reserved
 *
 ****************************************************************************/

#ifndef APP_ZIGBEENODECONTROLBRIDGE_CMDS_H_
#define APP_ZIGBEENODECONTROLBRIDGE_CMDS_H_

#include <PWRM.h>

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

PUBLIC void vSendStatus(uint8 u8Status, uint8 u8SeqNum, const char *pcMessage);
PUBLIC void vProcessIncomingSerialCommands(void);
/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Inlined Functions                                            ***/
/****************************************************************************/

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

#endif /* APP_ZIGBEENODECONTROLBRIDGE_CMDS_H_ */
