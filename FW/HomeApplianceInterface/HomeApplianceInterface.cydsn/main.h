/*******************************************************************************
* Project Name:      PSoC 4 BLE Home Appliance Interface
* File Name:         main.h
********************************************************************************
* Description:
*  Contains defines for all Coop Loop operations and system level settings.
*  Single point for inlcuding all header files.
*
********************************************************************************
*/

#ifndef MAIN_HEADER
#define MAIN_HEADER

/* System Includes */
#include <project.h>
#include "Sleep.h"
#include "SystemUtils.h"
#include "ErrorLog.h"
#include "WatchdogTimer.h"

/* Library Includes */
#include <stdbool.h>

/* Add your process header files here and assign a process ID */

/* v------------- ADD YOUR PROCESS HERE -------------v */
#include "Batt.h"
#define BATT_PROCESS_ID              (0u)
    
#include "BLE.h"
#define BLE_PROCESS_ID               (1u)
    
#include "LED.h"
#define LED_PROCESS_ID               (2u)
    
#include "TOUCH.h"
#define TOUCH_PROCESS_ID             (3u)
    
#include "SLEEP.h"
#define SLEEP_PROCESS_ID             (4u)    
/* ^------------- ADD YOUR PROCESS HERE -------------^ */

/* Update the number of processes to match the number of processes in your project */
    
/* v------------- UPDATE THIS VALUE -------------v */
#define NUMBER_OF_PROCESSES         (5u)
/* ^------------- UPDATE THIS VALUE -------------^ */

#define ProjectMajorVersion         (0u)
#define ProjectMinorVersion         (0u)

/***************************************
*        SYSTEM DEFINES                *
****************************************/
#define SYSTEM_TICK_TIME_MS             (10u)   
    
/* Enable or disable the firmware testmux outputs */
#define PROCESS_DEBUG_ENABLED       (0u)
#define ENABLE_SLEEP                (1u)
#define PROCESS_UART_ENABLE         (0u)
/* do not move this #include.  PROCESS_DEBUG_ENABLED must be defined before this file is included */
#include "TestMux.h"
    
/* if you choose to alter the names of the queue varaibles in main.c
update these names to reflect the change */
#define QUEUE_NAME                  ActiveQueue_Flags
#define NEXTTICK_NAME               NextTick_Flags
#define DISABLE_SLEEP_NAME          DisableSleep_Flags
#define DISABLE_DEEPSLEEP_NAME      DisableDeepSleep_Flags    
    
void System_TestMux_Init(void);

#if (PROCESS_DEBUG_ENABLED == 1u)
    extern uint8 * System_DebugOutput;
#endif

/* system firmware test mux definitions */ 
#define DEBUG_COOP_TICK_MASK        (0x01)
#define DEBUG_COOP_LOOP_MASK        (0x02)
#define DEBUG_SLEEP_MASK            (0x04)
#define DEBUG_ALTACTIVE_MASK        (0x08)
#define DEBUG_WHILE_WAIT_MASK       (0x10)
#define DEBUG_ERROR_LOGGED_MASK     (0x20)
#define DEBUG_ERROR_MASK            (0x40)

/* system error definitions */
#define SYSTEM_ERROR_FAILED_TO_REGISTER_TESTMUX         (0x00)
#define SYSTEM_ERROR_FAILED_TO_SWITCH_FIRMWAREMUX       (0x01)
#define SYSTEM_ERROR_FAILED_TO_SWITCH_HARDWAREMUX       (0x02)

#define ENABLED                     (0x01)
#define DISABLED                    (0x00)

/* ADC Channel Defines */
#define ADC_EDA_CHAN1               (0x00)
#define ADC_TEMP_CHAN1              (0x01)              /* Ref */
#define ADC_TEMP_CHAN2              (0x02)              /* Thermistor */
#define ADC_TEMP_CHAN3              (0x03)              /* Offset */
#define ADC_BATT_CHAN0              (0x04)

#if(NUMBER_OF_PROCESSES > 32u)
    #error The maximum number of processes allowed is 32
#elif(NUMBER_OF_PROCESSES > 16u)
    typedef uint32                      QueueType;
    #define NUMBER_OF_PROCESSES_MAX     (32u)
#elif (NUMBER_OF_PROCESSES > 8u)
    typedef uint16                      QueueType;
    #define NUMBER_OF_PROCESSES_MAX     (16u)
#else
    typedef uint8                       QueueType;
    #define NUMBER_OF_PROCESSES_MAX     (8u)
#endif

#if (NUMBER_OF_PROCESSES_MAX == 8)
    #define SYSTEM_PROCESS_ID           (8u)
    #define TESTMUX_PROCESS_ID          (9u)
#elif (NUMBER_OF_PROCESSES_MAX == 16)
    #define SYSTEM_PROCESS_ID           (16u)
    #define TESTMUX_PROCESS_ID          (17u)
#elif (NUMBER_OF_PROCESSES_MAX == 32)
    #define SYSTEM_PROCESS_ID           (32u)
    #define TESTMUX_PROCESS_ID          (33u)
#endif

/* Wakeup Sources.  These defines are to be used as bit flags for the
 * wakeup source variable
*/
#define COOP_TICK                        (0x01)
#define CSD_SCAN                         (0x02)

extern uint8 CoOpTick; 
extern QueueType ActiveQueue_Flags;
extern QueueType NextTick_Flags;
extern QueueType DisableSleep_Flags;
extern QueueType DisableDeepSleep_Flags;
extern uint8 WakeupSource;
extern MUTEX ADC_Mutex;

#endif
