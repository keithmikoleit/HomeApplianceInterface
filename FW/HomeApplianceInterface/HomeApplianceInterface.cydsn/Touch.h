/*******************************************************************************
* Project Name:      PSoC 4 BLE Home Appliance Interface
* File Name:         Touch.h
********************************************************************************
* Description:
*  Contains defines, function prototypes, and macros for the Touch process.
*
********************************************************************************
*/
#ifndef TOUCH_H
#define TOUCH_H

#include "main.h"

/* The process header file and mask need to be added to main.h */
#define TOUCH_PROCESS_MASK                            ((QueueType)1u << TOUCH_PROCESS_ID)

/* Touch active and idle scan periods must be even multiples of eachother and
 * of the Wrist detect period */
#define TOUCH_ENABLE_INIT                             (0u)
#define TOUCH_ACTIVE_SCAN_PERIOD_MS                   (10)
/* How often in system ticks this process should run with an active touch */
#define TOUCH_ACTIVE_SCAN_PERIOD                      (TOUCH_ACTIVE_SCAN_PERIOD_MS / SYSTEM_TICK_TIME_MS)
#define TOUCH_IDLE_SCAN_PERIOD_MS                     (100)    
/* How often in system ticks this process should run with no active touch */
#define TOUCH_IDLE_SCAN_PERIOD                        (TOUCH_IDLE_SCAN_PERIOD_MS / SYSTEM_TICK_TIME_MS)

#define S_TOUCH_STATE_INIT                            (TOUCH_STARTSCAN)

/* Extern declerations */
extern uint16 Touch_Timer_Count;
extern uint16 Touch_Period;
extern uint8 Touch_Enable;

/* EDA Output Data Struct */
typedef struct{
    uint8 CurrentCentroid;
    uint8 Data_Ready;
}Touch_Output;
extern Touch_Output TouchResult;    
    
/* Error definitions.  keep the PROCESSNAME_ERROR_DESCRIPTION format for error log parsing */
#define TOUCH_ERROR_DEFAULT_STATE                     (0u)
#define TOUCH_ERROR_FAILED_TO_REGISTER_TESTMUX        (1u)
#define TOUCH_ERROR_2                                 (2u)
#define TOUCH_ERROR_3                                 (3u)

/* Test mux definitions */
#define TOUCH_DEBUG_ENTER_SM                          (0x01)
#define TOUCH_DEBUG_START_SCAN                        (0x02)
#define TOUCH_DEBUG_WAIT_FOR_SCAN                     (0x04)
#define TOUCH_DEBUG_PROCESS_RESULTS                   (0x08)

/* Touch State Machine */    
typedef enum _TOUCH_STATE
{
    TOUCH_STARTSCAN = 0u,
    TOUCH_IS_SCAN_COMPLETE,
    TOUCH_PROCESS_RESULTS,
} T_TOUCH_STATE;

/* Process Defines */
/***************************************
*    TUNABLE GESTURE PARAMETERS        *
****************************************/
#define MIN_TAP_TIMEOUT                 (1u)
#define MAX_TAP_TIMEOUT                 (50u)
#define MAX_TAP_DISTANCE                (15u)
#define MIN_SWIPE_TIMEOUT               (1u)
#define MAX_SWIPE_TIMEOUT               (50u)
#define MIN_SWIPE_DISTANCE              (30u)
#define LARGE_OBJECT_DEBOUNCE           (10u)   
#define ACTIVE_POWER_TIMEOUT_MS         (3000)
#define CUSTOM_CAPSENSE_FILTER          (1u)

/***************************************
*         SENSOR MASKS                 *
****************************************/
#define NO_TOUCH                        (0xFF)  /* Centroid = 0xFF for No Touch */
#define SENSOR_MASK                     (0x1F)  /* Four sensors + 1 ganged sensor for low power */
#define SLIDER_MASK                     (0x0F)  /* Four slider sensors */
#define GANGED_MASK                     (0x10)  /* Ganged sensor */

/***************************************
*         GESTURE CODES                *
****************************************/
#define NO_GESTURE                      (0x00)
#define TAP_GESTURE                     (0x01)
#define SWIPE_LEFT_GESTURE              (0x02)
#define SWIPE_RIGHT_GESTURE             (0x03)
#define LARGE_OBJECT                    (0xFF)
#define DIRECTION_LEFT                  (0x00)
#define DIRECTION_RIGHT                 (0x01)

/* Function Prototypes */

/* Coop Loop Functions */
void Touch_Process_Init(void);
void Touch_Process_Update(void);
void Touch_Process(void);

/* Process Specific Functions */
uint8 GetGesture(void);
    
/* Macros */
/* Only call Touch_Process_Update() if it is enabled */

#define mTouch_ProcessTimer_Update()\
    do\
    {\
        if(Touch_Enable)\
        {\
            Touch_Timer_Count--;\
            if(Touch_Timer_Count == 0u)\
            {\
                QUEUE_NAME |= TOUCH_PROCESS_MASK;\
                Touch_Timer_Count = Touch_Period;\
            }\
        }\
    } while (0)
    
/* Only call Touch_Process() if it is queued */
#define mTouch_Process()\
    do\
    {\
        if((QUEUE_NAME & TOUCH_PROCESS_MASK))\
        {\
            Touch_Process();\
        }\
    } while (0)

/* Enables the Touch Process */
#define mTouch_EnableProcess()\
    do\
    {\
        Touch_Enable = TOUCH_ENABLED;\
    } while(0)
    
/* Disables the Touch Process */
#define mTouch_DisableProcess()\
    do\
    {\
        Touch_Enable = TOUCH_DISABLED;\
    } while(0)
    
/* On the next run through the co-op loop, go to the destination next state */
#define mTouch_SetNextState(DESTINATION_STATE)\
    do\
    {\
        s_Touch_State = DESTINATION_STATE;\
    } while(0)

/* the do nothing macro */
#define mTouch_Continue()\
    do\
    {\
    } while(0)
    
#define mTouch_ExecuteOnNextCoOp() mTouch_Continue()
#define mTouch_RepeatOnNextCoOp() mTouch_Continue()

#define mTouch_ExecuteStateOnNextCoOp(DESTINATION_STATE)\
    do\
    {\
        mTouch_SetNextState(DESTINATION_STATE);\
        mTouch_ExecuteOnNextCoOp();\
    } while(0)

/* De-Queue and on the next tick, go to the destination state */
#define mTouch_NextTick()\
    do\
    {\
        NEXTTICK_NAME |= TOUCH_PROCESS_MASK;\
        mTouch_DeQueue();\
    } while(0)
    
#define mTouch_ExecuteOnNextTick()   mTouch_NextTick()
#define mTouch_RepeatOnNextTick()   mTouch_NextTick()

#define mTouch_ExecuteStateOnNextTick(DESTINATION_STATE)\
    do\
    {\
        mTouch_SetNextState(DESTINATION_STATE);\
        mTouch_ExecuteOnNextTick();\
    } while(0)

/* De-queue the process and when the process timer expires,
go to the destination state */
#define mTouch_DeQueue()\
    do\
    {\
        QUEUE_NAME &= ~TOUCH_PROCESS_MASK;\
    } while(0)

#define mTouch_ExecuteOnNextPeriod() mTouch_DeQueue()
#define mTouch_RepeatOnNextPeriod() mTouch_DeQueue()

#define mTouch_ExecuteStateOnNextPeriod(DESTINATION_STATE)\
    do\
    {\
        mTouch_SetNextState(DESTINATION_STATE);\
        mTouch_ExecuteOnNextPeriod();\
    } while(0)
    
/* The mTouch_ExecuteThisStateXTimes8() macro will repeat the same state 
Multiple times through the co-op loop until the desired number of repeats has been reached.
The maximum number of state repeats is 255 and the minimum is 1.
When the number of repeats has been met, it will move on to the next state */
#define mTouch_ExecuteThisStateXTimes8(REPEAT, REPEAT_ACTION, DESTINATION_STATE, DESTINATION_ACTION)\
    do\
    {\
        if(Touch_SleepCountInit == TOUCH_NOT_INITIALIZED)\
        {\
            Touch_SleepCounter8 = REPEAT - 1u;\
            Touch_SleepCountInit = TOUCH_INITIALIZED;\
        }\
        if(Touch_SleepCounter8 == 0u)\
        {\
            mTouch_SetNextState(DESTINATION_STATE);\
            mTouch_##DESTINATION_ACTION();\
            Touch_SleepCountInit = TOUCH_NOT_INITIALIZED;\
        }\
        else\
        {\
            Touch_SleepCounter8--;\
            mTouch_##REPEAT_ACTION();\
        }\
    } while(0)
    
/* The mTouch_ExecuteThisStateXTimes16() macro will repeat the same state 
Multiple times through the co-op loop until the desired number of repeats has been reached.
The maximum number of state repeats is 65535 and the minimum is 1.
When the number of repeats has been met, it will move on to the next state */
#define mTouch_ExecuteThisStateXTimes16(REPEAT, REPEAT_ACTION, DESTINATION_STATE, DESTINATION_ACTION)\
    do\
    {\
        if(Touch_SleepCountInit == TOUCH_NOT_INITIALIZED)\
        {\
            Touch_SleepCounter16 = REPEAT - 1u;\
            Touch_SleepCountInit = TOUCH_INITIALIZED;\
        }\
        if(Touch_SleepCounter16 == 0u)\
        {\
            mTouch_SetNextState(DESTINATION_STATE);\
            mTouch_##DESTINATION_ACTION();\
            Touch_SleepCountInit = TOUCH_NOT_INITIALIZED;\
        }\
        else\
        {\
            Touch_SleepCounter16--;\
            mTouch_##REPEAT_ACTION();\
        }\
    } while(0)
    
/* The mTouch_SleepProcess() macro will sleep the process for the
desired number of ticks, preventing any execution of the process
until the number of ticks has been reached.  When the desired
number of ticks has elapsed, the state machine will execute the
destination state.  This macro temporarily overrides the 
process timer and sets it to the desired number of Ticks.
The process timer will return to its original period when 
the sleep period has ended */
#define mTouch_SleepProcess(TICKS, DESTINATION_STATE)\
    do\
    {\
        mTouch_SetNextState(DESTINATION_STATE);\
        Touch_Timer_Count = TICKS;\
        mTouch_DeQueue();\
    } while(0)

/* This process is OK with the device going to sleep */
#define mTouch_AllowSleep()\
    do\
    {\
        DISABLE_SLEEP_NAME &= ~TOUCH_PROCESS_MASK;\
    } while(0)

/* This process will block the device from going to sleep, but allow alt active */
#define mTouch_DisallowSleep()\
    do\
    {\
        DISABLE_SLEEP_NAME |= TOUCH_PROCESS_MASK;\
    } while(0)
    
/* This process is OK with the device going to deep sleep */
#define mTouch_AllowDeepSleep()\
    do\
    {\
        DISABLE_DEEPSLEEP_NAME &= ~TOUCH_PROCESS_MASK;\
    } while(0)

/* This process will block the device from going to deep sleep, but allow alt active */
#define mTouch_DisallowDeepSleep()\
    do\
    {\
        DISABLE_DEEPSLEEP_NAME |= TOUCH_PROCESS_MASK;\
    } while(0)


/* Defines for Touch Process */
#define TOUCH_ENABLED                         (0xFF)
#define TOUCH_DISABLED                        (0u)

#define TOUCH_INITIALIZED                     (0xFF)
#define TOUCH_NOT_INITIALIZED                 (0u)

#endif
/* [] END OF FILE */
