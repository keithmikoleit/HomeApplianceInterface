/*******************************************************************************
* Project Name:      PSoC 4 BLE Home Appliance Interface
* File Name:         WatchdogTimer.h
********************************************************************************
* Description:
*  Contains defines, function prototypes, and macros for the Watchdog Timer.
*
********************************************************************************
*/
#if !defined (_WATCHDOG_TIMER_H)
#define _WATCHDOG_TIMER_H

    
/*****************************************************************************
* Included headers
*****************************************************************************/
#include "main.h"

/*****************************************************************************
* Public functions
*****************************************************************************/
CY_ISR_PROTO(WatchdogTimer_Isr);
extern void WatchdogTimer_Init(void);
uint32 WatchdogTimer_GetTimestamp(void);

/*****************************************************************************
* Public variables
*****************************************************************************/

#endif

/* [] END OF FILE */
