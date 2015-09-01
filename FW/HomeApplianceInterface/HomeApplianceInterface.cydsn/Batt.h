/*******************************************************************************
* Project Name:      PSoC 4 BLE Home Appliance Interface
* File Name:         Batt.h
********************************************************************************
* Description:
*  Contains defines, function prototypes, and macros for the battery
*  measurement process.
*
********************************************************************************
*/
#ifndef BATT_H
#define BATT_H

#include "main.h"

/* The process header file and mask need to be added to main.h */
#define BATT_PROCESS_MASK                            ((QueueType)1u << BATT_PROCESS_ID)

/* How often in system ticks this process should run */
#define BATT_ENABLE_INIT                             (1u)
#define BATT_PROCESS_PERIOD_INIT                     (100u)      
#define S_BATT_STATE_INIT                            (BATT_STATE_START)

/* Extern declerations */
extern uint16 Batt_Timer_Count;
extern uint16 Batt_Period;
extern uint8 Batt_Enable;
    
/* Battery Output Data Struct */
typedef struct{
    uint8 Batt_Level;
    uint8 Data_Ready;
}Batt_Output;
extern Batt_Output BattResult;

/* Error definitions.  keep the PROCESSNAME_ERROR_DESCRIPTION format for error log parsing */
#define BATT_ERROR_DEFAULT_STATE                     (0u)
#define BATT_ERROR_FAILED_TO_REGISTER_TESTMUX        (1u)
#define BATT_ERROR_2                                 (2u)
#define BATT_ERROR_3                                 (3u)

/* Test mux definitions */
#define BATT_DEBUG_ENTER_SM                          (0x01)
#define BATT_DEBUG_2                                 (0x02)
#define BATT_DEBUG_3                                 (0x04)
#define BATT_DEBUG_4                                 (0x08)
#define BATT_DEBUG_5                                 (0x10)
#define BATT_DEBUG_6                                 (0x20)
#define BATT_DEBUG_7                                 (0x40)
#define BATT_DEBUG_8                                 (0x80)

/* ADC Reconfiguration Register Constants */
#define ADC_SAR_VREF_MASK   (0x00000070Lu)
#define ADC_SAR_VNEG_MASK   (0x00000E00Lu)
#define ADC_BYPASS_EN_MASK  (0x00000080Lu)

/* Calculation Constants */
#define BATT_SETTLING_TICKS                         (1u)  
#define MAX_BATTERY_VOLTAGE_MV                      (3300u)
#define MIN_BATTERY_VOLTAGE_MV                      (2000u)
#define BATT_R1                                     (30000u)
#define BATT_R2                                     (10000u)
#define ADC_VREF_MV                                 (1024u)
#define RESISTOR_SCALE                              (ADC_VREF_MV*(BATT_R1+BATT_R2)/BATT_R2)
    
typedef enum _BATT_STATE
{
    BATT_STATE_START = 0u,
    BATT_STATE_WAIT_ANALOG,
    BATT_STATE_START_ADC,
    BATT_STATE_WAIT_ADC,
    BATT_STATE_FREE_ANALOG,
    BATT_STATE_POST_PROCESS
    
} T_BATT_STATE;

/* Function Prototypes */
void Batt_Process_Init(void);
void Batt_Process_Update(void);
void Batt_Process(void);
    
/* Macros */
/* Only call Batt_Process_Update() if it is enabled */
#define mBatt_ProcessTimer_Update()\
    do\
    {\
        if(Batt_Enable)\
        {\
            Batt_Timer_Count--;\
            if(Batt_Timer_Count == 0u)\
            {\
                QUEUE_NAME |= BATT_PROCESS_MASK;\
                Batt_Timer_Count = Batt_Period;\
            }\
        }\
    } while (0)
    
/* Only call Batt_Process() if it is queued */
#define mBatt_Process()\
    do\
    {\
        if((QUEUE_NAME & BATT_PROCESS_MASK))\
        {\
            Batt_Process();\
        }\
    } while (0)

/* Enables the Batt Process */
#define mBatt_EnableProcess()\
    do\
    {\
        Batt_Enable = BATT_ENABLED;\
    } while(0)
    
/* Disables the Batt Process */
#define mBatt_DisableProcess()\
    do\
    {\
        Batt_Enable = BATT_DISABLED;\
    } while(0)
    
/* On the next run through the co-op loop, go to the destination next state */
#define mBatt_SetNextState(DESTINATION_STATE)\
    do\
    {\
        s_Batt_State = DESTINATION_STATE;\
    } while(0)

/* the do nothing macro */
#define mBatt_Continue()\
    do\
    {\
    } while(0)
    
#define mBatt_ExecuteOnNextCoOp() mBatt_Continue()
#define mBatt_RepeatOnNextCoOp() mBatt_Continue()

#define mBatt_ExecuteStateOnNextCoOp(DESTINATION_STATE)\
    do\
    {\
        mBatt_SetNextState(DESTINATION_STATE);\
        mBatt_ExecuteOnNextCoOp();\
    } while(0)

/* De-Queue and on the next tick, go to the destination state */
#define mBatt_NextTick()\
    do\
    {\
        NEXTTICK_NAME |= BATT_PROCESS_MASK;\
        mBatt_DeQueue();\
    } while(0)
    
#define mBatt_ExecuteOnNextTick()   mBatt_NextTick()
#define mBatt_RepeatOnNextTick()   mBatt_NextTick()

#define mBatt_ExecuteStateOnNextTick(DESTINATION_STATE)\
    do\
    {\
        mBatt_SetNextState(DESTINATION_STATE);\
        mBatt_ExecuteOnNextTick();\
    } while(0)

/* De-queue the process and when the process timer expires,
go to the destination state */
#define mBatt_DeQueue()\
    do\
    {\
        QUEUE_NAME &= ~BATT_PROCESS_MASK;\
    } while(0)

#define mBatt_ExecuteOnNextPeriod() mBatt_DeQueue()
#define mBatt_RepeatOnNextPeriod() mBatt_DeQueue()

#define mBatt_ExecuteStateOnNextPeriod(DESTINATION_STATE)\
    do\
    {\
        mBatt_SetNextState(DESTINATION_STATE);\
        mBatt_ExecuteOnNextPeriod();\
    } while(0)
    
/* The mBatt_ExecuteThisStateXTimes8() macro will repeat the same state 
Multiple times through the co-op loop until the desired number of repeats has been reached.
The maximum number of state repeats is 255 and the minimum is 1.
When the number of repeats has been met, it will move on to the next state */
#define mBatt_ExecuteThisStateXTimes8(REPEAT, REPEAT_ACTION, DESTINATION_STATE, DESTINATION_ACTION)\
    do\
    {\
        if(Batt_SleepCountInit == BATT_NOT_INITIALIZED)\
        {\
            Batt_SleepCounter8 = REPEAT - 1u;\
            Batt_SleepCountInit = BATT_INITIALIZED;\
        }\
        if(Batt_SleepCounter8 == 0u)\
        {\
            mBatt_SetNextState(DESTINATION_STATE);\
            mBatt_##DESTINATION_ACTION();\
            Batt_SleepCountInit = BATT_NOT_INITIALIZED;\
        }\
        else\
        {\
            Batt_SleepCounter8--;\
            mBatt_##REPEAT_ACTION();\
        }\
    } while(0)
    
/* The mBatt_ExecuteThisStateXTimes16() macro will repeat the same state 
Multiple times through the co-op loop until the desired number of repeats has been reached.
The maximum number of state repeats is 65535 and the minimum is 1.
When the number of repeats has been met, it will move on to the next state */
#define mBatt_ExecuteThisStateXTimes16(REPEAT, REPEAT_ACTION, DESTINATION_STATE, DESTINATION_ACTION)\
    do\
    {\
        if(Batt_SleepCountInit == BATT_NOT_INITIALIZED)\
        {\
            Batt_SleepCounter16 = REPEAT - 1u;\
            Batt_SleepCountInit = BATT_INITIALIZED;\
        }\
        if(Batt_SleepCounter16 == 0u)\
        {\
            mBatt_SetNextState(DESTINATION_STATE);\
            mBatt_##DESTINATION_ACTION();\
            Batt_SleepCountInit = BATT_NOT_INITIALIZED;\
        }\
        else\
        {\
            Batt_SleepCounter16--;\
            mBatt_##REPEAT_ACTION();\
        }\
    } while(0)
    
/* The mBatt_SleepProcess() macro will sleep the process for the
desired number of ticks, preventing any execution of the process
until the number of ticks has been reached.  When the desired
number of ticks has elapsed, the state machine will execute the
destination state.  This macro temporarily overrides the 
process timer and sets it to the desired number of Ticks.
The process timer will return to its original period when 
the sleep period has ended */
#define mBatt_SleepProcess(TICKS, DESTINATION_STATE)\
    do\
    {\
        mBatt_SetNextState(DESTINATION_STATE);\
        Batt_Timer_Count = TICKS;\
        mBatt_DeQueue();\
    } while(0)

/* This process is OK with the device going to sleep */
#define mBatt_AllowSleep()\
    do\
    {\
        DISABLE_SLEEP_NAME &= ~BATT_PROCESS_MASK;\
    } while(0)

/* This process will block the device from going to sleep, but allow alt active */
#define mBatt_DisallowSleep()\
    do\
    {\
        DISABLE_SLEEP_NAME |= BATT_PROCESS_MASK;\
    } while(0)

/* This process is OK with the device going to deep sleep */
#define mBatt_AllowDeepSleep()\
    do\
    {\
        DISABLE_DEEPSLEEP_NAME &= ~BATT_PROCESS_MASK;\
    } while(0)

/* This process will block the device from going to deep sleep */
#define mBatt_DisallowDeepSleep()\
    do\
    {\
        DISABLE_DEEPSLEEP_NAME |= BATT_PROCESS_MASK;\
    } while(0)    
    
/* Defines for Batt Process */
#define BATT_ENABLED                         (0xFF)
#define BATT_DISABLED                        (0u)

#define BATT_INITIALIZED                     (0xFF)
#define BATT_NOT_INITIALIZED                 (0u)

#endif
/* [] END OF FILE */
