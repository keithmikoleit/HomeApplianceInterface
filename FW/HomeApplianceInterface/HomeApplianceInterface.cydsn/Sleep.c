/*******************************************************************************
* Project Name:      PSoC 4 BLE Home Appliance Interface
* File Name:         Sleep.c
********************************************************************************
* Description:
*  The sleep process is responsible for changing the chip level power mode.
*  All power mode changes are done from within this process.
*
********************************************************************************
*/
#include "Sleep.h"

#if (PROCESS_DEBUG_ENABLED == 1u)
    uint8 * Sleep_DebugOutput;
#endif

void Sleep_Init(void)
{
    #if (PROCESS_DEBUG_ENABLED == 1u)
    if(TestMux_Register(SLEEP_PROCESS_ID, &Sleep_DebugOutput)  == TESTMUX_FAIL)
    {
        Log_Error(SLEEP_PROCESS_ID, SLEEP_ERROR_FAILED_TO_REGISTER_TESTMUX);
    }
    #endif
}

void Sleep_Process(void)
{
    CYBLE_BLESS_STATE_T bleMode;
    uint8 interruptStatus;
    
    #if (PROCESS_DEBUG_ENABLED == 1u)
        uint8 CYDATA TextMuxHardwareRestore;
        uint8 CYDATA TextMuxFirmware0Restore;
        uint8 CYDATA TextMuxFirmware1Restore;
    #endif
    /* save the hardware test mux control register value */
    
    #if (PROCESS_DEBUG_ENABLED == 1u)
        TextMuxHardwareRestore = HardwareDebugMuxSelect_Control;
        TextMuxFirmware0Restore = FirmwareDebugOutput0_Control;
        TextMuxFirmware1Restore = FirmwareDebugOutput1_Control;
    #endif
    

    /* Put the device to deep sleep */
    /* The idea of low power operation is to first request the BLE 
     * block go to Deep Sleep, and then check whether it actually
     * entered Deep Sleep. This is important because the BLE block
     * runs asynchronous to the rest of the application and thus
     * could be busy/idle independent of the application state. 
     * 
     * Once the BLE block is in Deep Sleep, only then the system 
     * can enter Deep Sleep. This is important to maintain the BLE 
     * connection alive while being in Deep Sleep.
     */
    
    mDebugSet(Sleep_DebugOutput, SLEEP_DEBUG_SLEEP_BLESS);
    
    /* Request the BLE block to enter Deep Sleep */
    CyBle_EnterLPM(CYBLE_BLESS_DEEPSLEEP);

    
    /* Check if the BLE block entered Deep Sleep and if so, then the 
     * system can enter Deep Sleep. This is done inside a Critical 
     * Section (where global interrupts are disabled) to avoid a 
     * race condition between application main (that wants to go to 
     * Deep Sleep) and other interrupts (which keep the device from 
     * going to Deep Sleep). 
     */
    interruptStatus = CyEnterCriticalSection();
    
    bleMode = CyBle_GetBleSsState();
    
    mDebugClear(Sleep_DebugOutput, SLEEP_DEBUG_SLEEP_BLESS);
    
    /* Also check if our system tick or touch interrupts have fired and set
     * there respective flags.  If so we cannot go to sleep because we must
     * return to the coop loop and let the associated processes run. */
    if(WakeupSource == 0)
    {   
        if((bleMode == CYBLE_BLESS_STATE_DEEPSLEEP || bleMode == CYBLE_BLESS_STATE_ECO_ON) &&
            DisableDeepSleep_Flags == 0u)
        {
            mDebugSet(Sleep_DebugOutput, SLEEP_DEBUG_DEEP_SLEEP);
			/* Enter Deep Sleep */
            CySysPmDeepSleep();
            mDebugClear(Sleep_DebugOutput, SLEEP_DEBUG_DEEP_SLEEP);
        }   
        /* The else condition signifies that the system cannot enter Deep Sleep.  
         */
        else if((bleMode != CYBLE_BLESS_STATE_EVENT_CLOSE))
        {
            /* The BLE block cannot enter deep sleep, but the rest of the system
             * is ready for deep sleep.  Therefore we switch to a the ECO clock
             * for the system and sleep the processor */          
            if(DisableDeepSleep_Flags == 0)
            {
                /* change HF clock source from IMO to ECO, as IMO is not required and can be stopped to save power */
                CySysClkWriteHfclkDirect(CY_SYS_CLK_HFCLK_ECO); 
                /* stop IMO for reducing power consumption */
                CySysClkImoStop();              
                /* put the CPU to sleep */
                mDebugSet(Sleep_DebugOutput, SLEEP_DEBUG_SLEEP_NO_IMO);
                CySysPmSleep();
                mDebugSet(Sleep_DebugOutput, SLEEP_DEBUG_SLEEP_NO_IMO);
                /* starts execution after waking up, start IMO */
                CySysClkImoStart();
                /* change HF clock source back to IMO */
                CySysClkWriteHfclkDirect(CY_SYS_CLK_HFCLK_IMO);    
            }
            
            /* The BLE block cannot enter deep sleep and the system requires
             * the IMO for hardware peripheral functionality.  We can sleep the
             * CPU. */
            else if(DisableSleep_Flags == 0)
            {   
                mDebugSet(Sleep_DebugOutput, SLEEP_DEBUG_SLEEP);
                CySysPmSleep();
                mDebugClear(Sleep_DebugOutput, SLEEP_DEBUG_SLEEP);
            }
            else
            {
                /* Sleep has been disabled, we are currently processing
                 * data that requires the CPU and peripherals be kept on.  We must
                 * simply wait for the next system tick before proceeding */
                
                /* TODO This case needs to be evaluated.  Due to BLE processing we
                   may have to exit this loop sooner than the next tick. The best
                   way to solve this in the system would be to have the BLE interrupt 
				   set a flag in the WakeupSource variable */
                
                /* Exit Critical section - Global interrupts are enabled again */
                CyExitCriticalSection(interruptStatus);
                
                /* wait for the next system tick note that the BLE interrupt
                   will not break us out of this loop */
                while(WakeupSource == 0){}
            }
        }  
    }
    
    /* Exit Critical section - Global interrupts are enabled again */
    CyExitCriticalSection(interruptStatus);
    
    #if (PROCESS_DEBUG_ENABLED == 1u)
        HardwareDebugMuxSelect_Control = TextMuxHardwareRestore;
        FirmwareDebugOutput0_Control = TextMuxFirmware0Restore;
        FirmwareDebugOutput1_Control = TextMuxFirmware1Restore;
    #endif

}


/* [] END OF FILE */

