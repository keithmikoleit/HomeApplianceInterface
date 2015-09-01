/*******************************************************************************
* Project Name:      PSoC 4 BLE Home Appliance Interface
* File Name:         Errorlog.h
********************************************************************************
* Description:
*  Contains defines, function prototypes, and macros for the error log.
*
********************************************************************************
*/

#ifndef ERRORLOG_HEADER
#define ERRORLOG_HEADER
    
#include "main.h"

#define MAX_NUMBER_OF_ERRORS    (128u)
#define ERROR_LOG_SIZE          (MAX_NUMBER_OF_ERRORS * 2u)

#if (ERROR_LOG_SIZE <= 256u)
    typedef uint8 ERROR_INDEX_TYPE;
#elif (ERROR_LOG_SIZE <= 65536u)
    typedef uint16 ERROR_INDEX_TYPE;
#else
    #error error log size too large
#endif
    
void Log_Error(uint8 ProcessID, uint8 Error);
void ClearLog(void);

#define mClearErrorPin()\
    do\
    {\
        mDebugClear(System_DebugOutput, DEBUG_ERROR_MASK);\
    }while(0)

extern uint8 ErrorLogArray[ERROR_LOG_SIZE];
extern ERROR_INDEX_TYPE ErrorIndex;
extern ERROR_INDEX_TYPE ErrorCount;
    
#endif

/* [] END OF FILE */
