/*******************************************************************************
* Project Name:      PSoC 4 BLE Home Appliance Interface
* File Name:         BLE.h
********************************************************************************
* Description:
*  Contains defines, function prototypes, and macros for the BLE process.
*
********************************************************************************
*/
#ifndef BLE_H
#define BLE_H

#include "main.h"

/* The process header file and mask need to be added to main.h */
#define BLE_PROCESS_MASK                            ((QueueType)1u << BLE_PROCESS_ID)

/* How often in system ticks this process should run */
#define BLE_ENABLE_INIT                             (1u)
#define BLE_PROCESS_PERIOD_INIT                     (1u)      
#define S_BLE_STATE_INIT                            (BLE_STATE_1)    
    
/* Extern declerations */
extern uint16 BLE_Timer_Count;
extern uint16 BLE_Period;
extern uint8 BLE_Enable;

/* Error definitions.  keep the PROCESSNAME_ERROR_DESCRIPTION format for error log parsing */
#define BLE_ERROR_DEFAULT_STATE                     (0u)
#define BLE_ERROR_FAILED_TO_REGISTER_TESTMUX        (1u)
#define BLE_ERROR_BAS_ERROR                         (2u)
#define BLE_ERROR_HTS_ERROR                         (3u)
#define BLE_ERROR_RSCS_ERROR                        (4u)

/* Test mux definitions */
#define BLE_DEBUG_ENTER_SM                          (0x01)
#define BLE_DEBUG_SEND_EDA                          (0x02)
#define BLE_DEBUG_3                                 (0x04)
#define BLE_DEBUG_4                                 (0x08)
#define BLE_DEBUG_5                                 (0x10)
#define BLE_DEBUG_6                                 (0x20)
#define BLE_DEBUG_7                                 (0x40)
#define BLE_DEBUG_8                                 (0x80)
    
/* Alert Levels */
#define NO_ALERT           (0u)
#define MILD_ALERT         (1u)
#define HIGH_ALERT         (2u)

/* EDA BLE Defines */
#define EDA_CHAR_DATA_LEN               (4u)
#define CCC_DATA_LEN                    (2u)
#define EDA_CCC_INDEX		            (0u)
#define EDA_INTERVAL_DATA_LEN           (2u)
    
/* Temperature BLE Defines */
#define TEMP_CHAR_DATA_LEN              (5u)     /* 1 byte flag + 4 byte temp */
    #define HTS_FLAGS_OFFSET            (0x00)
    #define HTS_TEMP_OFFSET             (0x01)

/* RSCS BLE Defines */
#define RSCS_CHAR_DATA_LEN              (4u)     /* 1 byte Flags + 
                                                    2 byte Speed +
                                                    1 byte Cadence */
#define RSCS_FLAGS_OFFSET               (0x00)
#define RSCS_SPEED_OFFSET               (0x01)
#define RSCS_CADENCE_OFFSET             (0x03)
    
typedef enum _BLE_STATE
{
    BLE_STATE_1 = 0u,
    BLE_STATE_2,
    BLE_STATE_3,
    BLE_STATE_4,
    BLE_STATE_5
    
} T_BLE_STATE;

/* Function Prototypes */
void BLE_Process_Init(void);
void BLE_Process_Update(void);
void BLE_Process(void);
uint8 GetAlertLevel(void);
    
/* Macros */
/* Only call BLE_Process_Update() if it is enabled */
/* The BLE process runs every time through the coop loop */
#define mBLE_ProcessTimer_Update()\
    do\
    {\
        if(BLE_Enable)\
        {\
            QUEUE_NAME |= BLE_PROCESS_MASK;\
        }\
    } while (0)
    
/* if enabled BLE_Process() runs every time through the coop
   loop as its requirements are asyncronous */
#define mBLE_Process()\
    do\
    {\
        if((QUEUE_NAME & BLE_PROCESS_MASK))\
        {\
            BLE_Process();\
        }\
    } while (0)

/* Enables the BLE Process */
#define mBLE_EnableProcess()\
    do\
    {\
        BLE_Enable = BLE_ENABLED;\
    } while(0)
    
/* Disables the BLE Process */
#define mBLE_DisableProcess()\
    do\
    {\
        BLE_Enable = BLE_DISABLED;\
    } while(0)
    
/* On the next run through the co-op loop, go to the destination next state */
#define mBLE_SetNextState(DESTINATION_STATE)\
    do\
    {\
        s_BLE_State = DESTINATION_STATE;\
    } while(0)

/* the do nothing macro */
#define mBLE_Continue()\
    do\
    {\
    } while(0)
    
#define mBLE_ExecuteOnNextCoOp() mBLE_Continue()
#define mBLE_RepeatOnNextCoOp() mBLE_Continue()

#define mBLE_ExecuteStateOnNextCoOp(DESTINATION_STATE)\
    do\
    {\
        mBLE_SetNextState(DESTINATION_STATE);\
        mBLE_ExecuteOnNextCoOp();\
    } while(0)

/* De-Queue and on the next tick, go to the destination state */
#define mBLE_NextTick()\
    do\
    {\
        NEXTTICK_NAME |= BLE_PROCESS_MASK;\
        mBLE_DeQueue();\
    } while(0)
    
#define mBLE_ExecuteOnNextTick()   mBLE_NextTick()
#define mBLE_RepeatOnNextTick()   mBLE_NextTick()

#define mBLE_ExecuteStateOnNextTick(DESTINATION_STATE)\
    do\
    {\
        mBLE_SetNextState(DESTINATION_STATE);\
        mBLE_ExecuteOnNextTick();\
    } while(0)

/* De-queue the process and when the process timer expires,
go to the destination state */
#define mBLE_DeQueue()\
    do\
    {\
        QUEUE_NAME &= ~BLE_PROCESS_MASK;\
    } while(0)

#define mBLE_ExecuteOnNextPeriod() mBLE_DeQueue()
#define mBLE_RepeatOnNextPeriod() mBLE_DeQueue()

#define mBLE_ExecuteStateOnNextPeriod(DESTINATION_STATE)\
    do\
    {\
        mBLE_SetNextState(DESTINATION_STATE);\
        mBLE_ExecuteOnNextPeriod();\
    } while(0)
    
/* The mBLE_ExecuteThisStateXTimes8() macro will repeat the same state 
Multiple times through the co-op loop until the desired number of repeats has been reached.
The maximum number of state repeats is 255 and the minimum is 1.
When the number of repeats has been met, it will move on to the next state */
#define mBLE_ExecuteThisStateXTimes8(REPEAT, REPEAT_ACTION, DESTINATION_STATE, DESTINATION_ACTION)\
    do\
    {\
        if(BLE_SleepCountInit == BLE_NOT_INITIALIZED)\
        {\
            BLE_SleepCounter8 = REPEAT - 1u;\
            BLE_SleepCountInit = BLE_INITIALIZED;\
        }\
        if(BLE_SleepCounter8 == 0u)\
        {\
            mBLE_SetNextState(DESTINATION_STATE);\
            mBLE_##DESTINATION_ACTION();\
            BLE_SleepCountInit = BLE_NOT_INITIALIZED;\
        }\
        else\
        {\
            BLE_SleepCounter8--;\
            mBLE_##REPEAT_ACTION();\
        }\
    } while(0)
    
/* The mBLE_ExecuteThisStateXTimes16() macro will repeat the same state 
Multiple times through the co-op loop until the desired number of repeats has been reached.
The maximum number of state repeats is 65535 and the minimum is 1.
When the number of repeats has been met, it will move on to the next state */
#define mBLE_ExecuteThisStateXTimes16(REPEAT, REPEAT_ACTION, DESTINATION_STATE, DESTINATION_ACTION)\
    do\
    {\
        if(BLE_SleepCountInit == BLE_NOT_INITIALIZED)\
        {\
            BLE_SleepCounter16 = REPEAT - 1u;\
            BLE_SleepCountInit = BLE_INITIALIZED;\
        }\
        if(BLE_SleepCounter16 == 0u)\
        {\
            mBLE_SetNextState(DESTINATION_STATE);\
            mBLE_##DESTINATION_ACTION();\
            BLE_SleepCountInit = BLE_NOT_INITIALIZED;\
        }\
        else\
        {\
            BLE_SleepCounter16--;\
            mBLE_##REPEAT_ACTION();\
        }\
    } while(0)
    
/* The mBLE_SleepProcess() macro will sleep the process for the
desired number of ticks, preventing any execution of the process
until the number of ticks has been reached.  When the desired
number of ticks has elapsed, the state machine will execute the
destination state.  This macro temporarily overrides the 
process timer and sets it to the desired number of Ticks.
The process timer will return to its original period when 
the sleep period has ended */
#define mBLE_SleepProcess(TICKS, DESTINATION_STATE)\
    do\
    {\
        mBLE_SetNextState(DESTINATION_STATE);\
        BLE_Timer_Count = TICKS;\
        mBLE_DeQueue();\
    } while(0)

/* This process is OK with the device going to sleep */
#define mBLE_AllowSleep()\
    do\
    {\
        DISABLE_SLEEP_NAME &= ~BLE_PROCESS_MASK;\
    } while(0)

/* This process will block the device from going to sleep, but allow alt active */
#define mBLE_DisallowSleep()\
    do\
    {\
        DISABLE_SLEEP_NAME |= BLE_PROCESS_MASK;\
    } while(0)

/* Defines for BLE Process */
#define BLE_ENABLED                         (0xFF)
#define BLE_DISABLED                        (0u)

#define BLE_INITIALIZED                     (0xFF)
#define BLE_NOT_INITIALIZED                 (0u)

#endif
/* [] END OF FILE */
