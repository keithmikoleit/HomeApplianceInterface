/*******************************************************************************
* Project Name:      PSoC 4 BLE Home Appliance Interface
* File Name:         LED.h
********************************************************************************
* Description:
*  Contains defines, function prototypes, and macros for the LED process.
*
********************************************************************************
*/
#ifndef LED_H
#define LED_H

#include "main.h"

/* The process header file and mask need to be added to main.h */
#define LED_PROCESS_MASK                            ((QueueType)1u << LED_PROCESS_ID)

/* How often in system ticks this process should run */
#define LED_ENABLE_INIT                             (1u)
#define LED_PROCESS_PERIOD_INIT                     (1u)      
#define S_LED_STATE_INIT                            (LED_STATE_1)

/* LED update source */
#define UPDATE_FROM_IAS_ALERT                       (0u)
#define UPDATE_FROM_TOUCH                           (1u)
#define UPDATE_FROM_EDA                              (2u)
#define LED_UPDATE_SOURCE                           (UPDATE_FROM_TOUCH)
    
/* Extern declerations */
extern uint16 LED_Timer_Count;
extern uint16 LED_Period;
extern uint8 LED_Enable;

/* Error definitions.  keep the PROCESSNAME_ERROR_DESCRIPTION format for error log parsing */
#define LED_ERROR_DEFAULT_STATE                     (0u)
#define LED_ERROR_FAILED_TO_REGISTER_TESTMUX        (1u)
#define LED_ERROR_2                                 (2u)
#define LED_ERROR_3                                 (3u)

/* Test mux definitions */
#define LED_DEBUG_ENTER_SM                          (0x01)
#define LED_DEBUG_2                                 (0x02)
#define LED_DEBUG_3                                 (0x04)
#define LED_DEBUG_4                                 (0x08)
#define LED_DEBUG_5                                 (0x10)
#define LED_DEBUG_6                                 (0x20)
#define LED_DEBUG_7                                 (0x40)
#define LED_DEBUG_8                                 (0x80)
    
typedef enum _LED_STATE
{
    LED_STATE_1 = 0u,
    LED_STATE_2,
    LED_STATE_3,
    LED_STATE_4,
    LED_STATE_5
    
} T_LED_STATE;

/* Function Prototypes */
void LED_Process_Init(void);
void LED_Process_Update(void);
void LED_Process(void);

/* defines for controling LEDs */
#define LED_OFF                         (1u)    /* Active High LEDs */
#define LED_ON                          (0u)    /* Active High LEDs */
#define LED_ON_TIME_MS                  (1000u) /* Time to turn on LED for each Gesture */
#define LED_On_TIME_TICKS               (LED_ON_TIME_MS / SYSTEM_TICK_TIME_MS)

/* Macros */
/* Only call LED_Process_Update() if it is enabled */
#define mLED_ProcessTimer_Update()\
    do\
    {\
        if(LED_Enable)\
        {\
            LED_Timer_Count--;\
            if(LED_Timer_Count == 0u)\
            {\
                QUEUE_NAME |= LED_PROCESS_MASK;\
                LED_Timer_Count = LED_Period;\
            }\
        }\
    } while (0)
    
/* Only call LED_Process() if it is queued */
#define mLED_Process()\
    do\
    {\
        if((QUEUE_NAME & LED_PROCESS_MASK))\
        {\
            LED_Process();\
        }\
    } while (0)

/* Enables the LED Process */
#define mLED_EnableProcess()\
    do\
    {\
        LED_Enable = LED_ENABLED;\
    } while(0)
    
/* Disables the LED Process */
#define mLED_DisableProcess()\
    do\
    {\
        LED_Enable = LED_DISABLED;\
    } while(0)
    
/* On the next run through the co-op loop, go to the destination next state */
#define mLED_SetNextState(DESTINATION_STATE)\
    do\
    {\
        s_LED_State = DESTINATION_STATE;\
    } while(0)

/* the do nothing macro */
#define mLED_Continue()\
    do\
    {\
    } while(0)
    
#define mLED_ExecuteOnNextCoOp() mLED_Continue()
#define mLED_RepeatOnNextCoOp() mLED_Continue()

#define mLED_ExecuteStateOnNextCoOp(DESTINATION_STATE)\
    do\
    {\
        mLED_SetNextState(DESTINATION_STATE);\
        mLED_ExecuteOnNextCoOp();\
    } while(0)

/* De-Queue and on the next tick, go to the destination state */
#define mLED_NextTick()\
    do\
    {\
        NEXTTICK_NAME |= LED_PROCESS_MASK;\
        mLED_DeQueue();\
    } while(0)
    
#define mLED_ExecuteOnNextTick()   mLED_NextTick()
#define mLED_RepeatOnNextTick()   mLED_NextTick()

#define mLED_ExecuteStateOnNextTick(DESTINATION_STATE)\
    do\
    {\
        mLED_SetNextState(DESTINATION_STATE);\
        mLED_ExecuteOnNextTick();\
    } while(0)

/* De-queue the process and when the process timer expires,
go to the destination state */
#define mLED_DeQueue()\
    do\
    {\
        QUEUE_NAME &= ~LED_PROCESS_MASK;\
    } while(0)

#define mLED_ExecuteOnNextPeriod() mLED_DeQueue()
#define mLED_RepeatOnNextPeriod() mLED_DeQueue()

#define mLED_ExecuteStateOnNextPeriod(DESTINATION_STATE)\
    do\
    {\
        mLED_SetNextState(DESTINATION_STATE);\
        mLED_ExecuteOnNextPeriod();\
    } while(0)
    
/* The mLED_ExecuteThisStateXTimes8() macro will repeat the same state 
Multiple times through the co-op loop until the desired number of repeats has been reached.
The maximum number of state repeats is 255 and the minimum is 1.
When the number of repeats has been met, it will move on to the next state */
#define mLED_ExecuteThisStateXTimes8(REPEAT, REPEAT_ACTION, DESTINATION_STATE, DESTINATION_ACTION)\
    do\
    {\
        if(LED_SleepCountInit == LED_NOT_INITIALIZED)\
        {\
            LED_SleepCounter8 = REPEAT - 1u;\
            LED_SleepCountInit = LED_INITIALIZED;\
        }\
        if(LED_SleepCounter8 == 0u)\
        {\
            mLED_SetNextState(DESTINATION_STATE);\
            mLED_##DESTINATION_ACTION();\
            LED_SleepCountInit = LED_NOT_INITIALIZED;\
        }\
        else\
        {\
            LED_SleepCounter8--;\
            mLED_##REPEAT_ACTION();\
        }\
    } while(0)
    
/* The mLED_ExecuteThisStateXTimes16() macro will repeat the same state 
Multiple times through the co-op loop until the desired number of repeats has been reached.
The maximum number of state repeats is 65535 and the minimum is 1.
When the number of repeats has been met, it will move on to the next state */
#define mLED_ExecuteThisStateXTimes16(REPEAT, REPEAT_ACTION, DESTINATION_STATE, DESTINATION_ACTION)\
    do\
    {\
        if(LED_SleepCountInit == LED_NOT_INITIALIZED)\
        {\
            LED_SleepCounter16 = REPEAT - 1u;\
            LED_SleepCountInit = LED_INITIALIZED;\
        }\
        if(LED_SleepCounter16 == 0u)\
        {\
            mLED_SetNextState(DESTINATION_STATE);\
            mLED_##DESTINATION_ACTION();\
            LED_SleepCountInit = LED_NOT_INITIALIZED;\
        }\
        else\
        {\
            LED_SleepCounter16--;\
            mLED_##REPEAT_ACTION();\
        }\
    } while(0)
    
/* The mLED_SleepProcess() macro will sleep the process for the
desired number of ticks, preventing any execution of the process
until the number of ticks has been reached.  When the desired
number of ticks has elapsed, the state machine will execute the
destination state.  This macro temporarily overrides the 
process timer and sets it to the desired number of Ticks.
The process timer will return to its original period when 
the sleep period has ended */
#define mLED_SleepProcess(TICKS, DESTINATION_STATE)\
    do\
    {\
        mLED_SetNextState(DESTINATION_STATE);\
        LED_Timer_Count = TICKS;\
        mLED_DeQueue();\
    } while(0)

/* This process is OK with the device going to sleep */
#define mLED_AllowSleep()\
    do\
    {\
        DISABLE_SLEEP_NAME &= ~LED_PROCESS_MASK;\
    } while(0)

/* This process will block the device from going to sleep, but allow alt active */
#define mLED_DisallowSleep()\
    do\
    {\
        DISABLE_SLEEP_NAME |= LED_PROCESS_MASK;\
    } while(0)
    
/* This process is OK with the device going to deep sleep */
#define mLED_AllowDeepSleep()\
    do\
    {\
        DISABLE_DEEPSLEEP_NAME &= ~LED_PROCESS_MASK;\
    } while(0)

/* This process will block the device from going to deep sleep, but allow alt active */
#define mLED_DisallowDeepSleep()\
    do\
    {\
        DISABLE_DEEPSLEEP_NAME |= LED_PROCESS_MASK;\
    } while(0)


/* Defines for LED Process */
#define LED_ENABLED                         (0xFF)
#define LED_DISABLED                        (0u)

#define LED_INITIALIZED                     (0xFF)
#define LED_NOT_INITIALIZED                 (0u)

#endif
/* [] END OF FILE */
