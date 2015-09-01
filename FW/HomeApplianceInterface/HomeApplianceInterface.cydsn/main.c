/*******************************************************************************
* Project Name:      PSoC 4 BLE Home Appliance Interface
* File Name:         main.c
********************************************************************************
* Description:
*  Initializes each process,  runs the co-operative time share loop 
*  and handles sleep/wakeup via the Sleep Process.
*
********************************************************************************
*/
#include <project.h>
#include "main.h"

/* There is no need to modify these variables */
/* These variables are bit fields where each bit corresponds to a process
The ActiveQueue indicates which processes are active for the current tick */
QueueType ActiveQueue_Flags = 0u;
/* The NextTick bit fields indicate which processes would like to be executed
on the next tick event */
QueueType NextTick_Flags = 0u;
/* The DisableSleep and DisableAltActive bitfields indicate which processes
cannot allow sleep or alt active modes */
QueueType DisableSleep_Flags = 0u;
QueueType DisableDeepSleep_Flags = 0u;
/* Bit flag variable used to identify wakeup source.  This is in turn
 * used to decide which process timers need to be updated */
uint8 WakeupSource;

/* Controls shared access to the ADC */
MUTEX ADC_Mutex = UNLOCKED;

/* If process debugging is enabled, define the debug pointer */
#if (PROCESS_DEBUG_ENABLED == 1u)
    uint8 * System_DebugOutput;
#endif

void main()
{
    CyGlobalIntEnable;
    
    /* low power and system initializations */
    Sleep_Init();
    #if (PROCESS_DEBUG_ENABLED == 1u)
    TestMux_Init();
    #endif
    WatchdogTimer_Init();

    /* System initializations */
    
    
    /* Internal low power oscillator is stopped as it is not used in this project */
    CySysClkIloStop();
    
    /* Set the divider for ECO, ECO will be used as source when IMO is switched off to save power */
    CySysClkWriteEcoDiv(CY_SYS_CLK_ECO_DIV8);
    
    /* Initialize added Processes. This executes any initialization that the processes
    need to run only once after a reset event */
    
    /* v------------- ADD YOUR PROCESS HERE -------------v */
    Batt_Process_Init();
    BLE_Process_Init();
    LED_Process_Init();
    Touch_Process_Init();
    /* ^------------- ADD YOUR PROCESS HERE -------------^ */
    
    /* Call after processes have been initialized so that their test mux
    outputs can be used */
    System_TestMux_Init();
    
    /* Sync to the system tick timer before entering the co-op loop for the
    first time */
//    CoOpTick = 0u;
//    while (CoOpTick == 0u){};
    WakeupSource = 0u;
    while((WakeupSource & COOP_TICK) == 0){};
    
    /* A tick event has occurred.
    Run the multi-tasking, process sharing co-operative loop */
    while(1u)
    {
        /* Process Timer updates. This allows each process to update its
        internal timer to decide if it needs to add itself to the queue */
        
        /* v------------- ADD YOUR PROCESS HERE -------------v */
        /* Only update processes if the system tick woke us up */
        if(WakeupSource & COOP_TICK)
        {            
            /* If process debugging is enabled, pulse the tick event pin */
            mDebugSet(System_DebugOutput, DEBUG_COOP_TICK_MASK);
            
            /* Place any NextTick processes into queue and then clear 
            the next tick requests */
            QUEUE_NAME |= NEXTTICK_NAME;
            NEXTTICK_NAME = 0u;
            
            /* Clear SysTick Flag (set in WatchdogTimer_Isr) 
             * This flag must be cleared only when the coop loop
             * sees that it is set, otherwise we will miss system ticks */
            //CoOpTick = 0u;
            WakeupSource &= ~COOP_TICK;
            
            /* ^------------- ADD YOUR PROCESS UPDATE HERE -------------^ */
            mBatt_ProcessTimer_Update();
            mLED_ProcessTimer_Update();
            mTouch_ProcessTimer_Update();
            mDebugClear(System_DebugOutput, DEBUG_COOP_TICK_MASK);
        }
        /* If a CSD scan woke us up run the touch process to finish up the scan
        */
        if(WakeupSource & CSD_SCAN)
        {
            QUEUE_NAME |= (TOUCH_PROCESS_MASK);
            WakeupSource &= ~CSD_SCAN;
        }       
        /* BLE runs everytime the system wakes up */
        mBLE_ProcessTimer_Update();
        
        /* Co-operative Loop
        Run all processes until queue is empty or we run out of time */
        while((ActiveQueue_Flags != 0u) && ((WakeupSource & COOP_TICK) == 0))
        {
            /* If process debugging is enabled, set the CoOp pin */
            mDebugSet(System_DebugOutput, DEBUG_COOP_LOOP_MASK);
            
            /* execute the processes */
            
            /* v------------- ADD YOUR PROCESS HERE -------------v */
            mBatt_Process();
            mBLE_Process();
            mLED_Process();
            mTouch_Process();
            /* ^------------- ADD YOUR PROCESS HERE -------------^ */
            
            /* If process debugging is enabled, clear the CoOp pin */
            mDebugClear(System_DebugOutput, DEBUG_COOP_LOOP_MASK);
        }
        
        #if(ENABLE_SLEEP == (1u))
        /* Go to sleep with remaining time, unless a tick has already occurred */
        if((WakeupSource & COOP_TICK) == 0)
        {   
            /* If process debugging is enabled, set the Sleep pin */
            mDebugSet(System_DebugOutput, DEBUG_SLEEP_MASK);
            
            /* enter Sleep mode */
            Sleep_Process();   
            
            /* If process debugging is enabled, clear the Sleep pin */
            mDebugClear(System_DebugOutput, DEBUG_SLEEP_MASK);
        }
        #else
        /* wait for the next system tick */
        while(WakeupSource == 0){}
        #endif
    }
}

void System_TestMux_Init(void)
{
    #if (PROCESS_DEBUG_ENABLED == 1u)
        /* register the system debug pointer with the test mux */
        if(TestMux_Register(SYSTEM_PROCESS_ID, &System_DebugOutput) == TESTMUX_FAIL)
        {
            /* if there was a failure during registration, log the error */
            Log_Error(SYSTEM_PROCESS_ID, SYSTEM_ERROR_FAILED_TO_REGISTER_TESTMUX);
        }
        
        /* DEBUG 0 - P3[4] */
        /* select a process to go to the control 0 debug output register  */
        if(TestMux_SelectProcess0(SYSTEM_PROCESS_ID) == TESTMUX_FAIL)
        {
            /* if there was a failure during switching the firmware debug pointer, log the error */
            Log_Error(SYSTEM_PROCESS_ID, SYSTEM_ERROR_FAILED_TO_SWITCH_FIRMWAREMUX);
        }
        
        /* select a hardware channel for the debug 0 output */
        if(TestMux_SelectSignal0(HARDWARE_CHANNEL1) == TESTMUX_FAIL)
        {
            /* if there was an error switching the hardware channel, log the error */
            Log_Error(SYSTEM_PROCESS_ID, SYSTEM_ERROR_FAILED_TO_SWITCH_HARDWAREMUX);
        }
        
        /* DEBUG 1 - P2[7] */
        /* select a process to go to the control 1 debug output register  */
        if(TestMux_SelectProcess1(BLE_PROCESS_ID) == TESTMUX_FAIL)
        {
            /* if there was a failure during switching the firmware debug pointer, log the error */
            Log_Error(SYSTEM_PROCESS_ID, SYSTEM_ERROR_FAILED_TO_SWITCH_FIRMWAREMUX);
        }
        
        /* select a hardware channel for the debug 1 output */
        if(TestMux_SelectSignal1(HARDWARE_CHANNEL1) == TESTMUX_FAIL)
        {
            /* if there was an error switching the hardware channel, log the error */
            Log_Error(SYSTEM_PROCESS_ID, SYSTEM_ERROR_FAILED_TO_SWITCH_HARDWAREMUX);
        }
    #endif
    
    return;
}
