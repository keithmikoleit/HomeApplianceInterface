/*******************************************************************************
* Project Name:      PSoC 4 BLE Home Appliance Interface
* File Name:         WatchdogTimer.c
********************************************************************************
* Description:
*  The watchdog timer generates our system tick.  This process is a system
*  level process and therefore does not follow the same structure as block
*  based processes.
*
********************************************************************************
*/

/*****************************************************************************
* Included headers
*****************************************************************************/
#include "WatchdogTimer.h"


/*****************************************************************************
* Macros and constants
*****************************************************************************/
#define WDT_PERIOD_MS               (SYSTEM_TICK_TIME_MS)
#define WDT_TICKS_PER_MS            (32)
#define WDT_TICKS                   (WDT_PERIOD_MS * WDT_TICKS_PER_MS)
#define WDT_INTERRUPT_NUM           (8)


/*****************************************************************************
* Static variables
*****************************************************************************/
uint16 WDT_Period = WDT_TICKS;     /* System wide WDT_Period */

/*****************************************************************************
* Public variables
*****************************************************************************/
/* This is the main system tick flag. Set by our regular tick event */
static uint32 watchdogTimestamp = 0;

/*****************************************************************************
* Public function definitions
*****************************************************************************/

/*******************************************************************************
* Function Name: WDT_Isr
********************************************************************************
*
* Summary:
*  Watchdog Timer ISR. The WDT and consequently, this ISR, control the system 
*   tick/update rate. Note that this ISR executing does not mean the WDT has
*   expired. This function updates the WDT_ISR_Flag and resets the WDT match
*   register, which requires reading the current value and adding the desired
*   period to that value and updating the WDT_MATCH_REG.
*
* Parameters:
*  None.
*
* Return:
*  None.
*
* Global Variables:
*  WDT_ISR_Flag - Informs main loop that a tick / WDT ISR has occurred.
*
*******************************************************************************/
CY_ISR(WatchdogTimer_Isr)
{   
	/* Set the WDT Timer ISR flag */
    //CoOpTick = 1;
    WakeupSource |= COOP_TICK;
    
    /* Update the system timestamp - the watchdog period time has elapsed
     * since the last interrupt.
     */
    watchdogTimestamp += WDT_PERIOD_MS;
	
	/* Clear WDT interrupt */
    CySysWdtClearInterrupt(CY_SYS_WDT_COUNTER0_INT);
}



/*****************************************************************************
* Function Name: WatchdogTimer_Start
******************************************************************************
* Summary:
* Starts the watchdog timer WDT0 to be used as the system timer. Define a 
* system timestamp variable to be updated on every watchdog interrupt.
*
* Parameters:
* None
*
* Return:
* None
*
* Theory:
* The function uses the watchdog timer WDT0 of the chip. It configures the 
* timer to fire an interrupt upon match. The periodic interrupt updates the 
* system timestamp variable which is used to keep track of the system activity.
* The timer is configured for clear on match i.e. the WDT counter is reset to
* zero upon a match event. The timer is continuously run.
*
* To change the watchdog timer settings, the function needs to unlock the 
* WDT first, and then lock it after the modification is complete.
*
* Side Effects:
* None
*
*****************************************************************************/
void WatchdogTimer_Init(void)
{
    /* Set the WDT ISR */
    CyIntSetVector(WDT_INTERRUPT_NUM, &WatchdogTimer_Isr);
    
    /* Unlock the sytem watchdog timer to be able to change settings */
	CySysWdtUnlock();
    
    /* Configure the watchdog timer 0 (WDT0) to fire an interrupt upon match 
     * i.e. when the count register value equals the match register value.
     */
    CySysWdtWriteMode(0, CY_SYS_WDT_MODE_INT);
    
    /* WDT0 counter to be cleared upon a match event and then begin again.
     * The timer is to be run continuously.
     */
	CySysWdtWriteClearOnMatch(0, 1);
    
    /* Set the value of the match register. Since the count starts from zero, 
     * the actual value is the (intended - 1). */
    CySysWdtWriteMatch(0, WDT_TICKS - 1);
    
    /* Enable the WDT0 */
    CySysWdtEnable(CY_SYS_WDT_COUNTER0_MASK);
    
    /* Enable interrupt */
    CyIntEnable(WDT_INTERRUPT_NUM);
    
    /* Lock the watchdog timer to prevent future modification */
	CySysWdtLock();
}

/*****************************************************************************
* Function Name: WatchdogTimer_GetTimestamp
******************************************************************************
* Summary:
* Returns the system timestamp value.
*
* Parameters:
* None
*
* Return:
* uint32: Current system timestamp 
*
* Theory:
* The function returns the watchdog timestamp.
*
* Side Effects:
* None
*
*****************************************************************************/
uint32 WatchdogTimer_GetTimestamp(void)
{
    return watchdogTimestamp;
}


/* [] END OF FILE */
