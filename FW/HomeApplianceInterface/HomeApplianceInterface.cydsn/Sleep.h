/*******************************************************************************
* Project Name:      PSoC 4 BLE Home Appliance Interface
* File Name:         Sleep.h
********************************************************************************
* Description:
*  Header file for the sleep process.  This process is a system level process
*  and therefore does not follow the normal block process structure.
*
********************************************************************************
*/
#ifndef SLEEP_H
#define SLEEP_H

#include "main.h"

/* Error definitions.  keep the PROCESSNAME_ERROR_DESCRIPTION format for error log parsing */
#define SLEEP_ERROR_DEFAULT_STATE                     (0u)
#define SLEEP_ERROR_FAILED_TO_REGISTER_TESTMUX        (1u)
#define SLEEP_ERROR_2                                 (2u)
#define SLEEP_ERROR_3                                 (3u)

/* Test mux definitions */
#define SLEEP_DEBUG_SLEEP_BLESS                       (0x01)
#define SLEEP_DEBUG_DEEP_SLEEP                        (0x02)
#define SLEEP_DEBUG_SLEEP                             (0x04)
#define SLEEP_DEBUG_SLEEP_NO_IMO                      (0x08)
#define SLEEP_DEBUG_5                                 (0x10)
#define SLEEP_DEBUG_6                                 (0x20)
#define SLEEP_DEBUG_7                                 (0x40)
#define SLEEP_DEBUG_8                                 (0x80)    
    
/* Function Prototypes */
void Sleep_Init(void);
void Sleep_Process(void);

#endif

//[] END OF FILE
