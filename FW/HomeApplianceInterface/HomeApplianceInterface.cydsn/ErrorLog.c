/*******************************************************************************
* Project Name:      PSoC 4 BLE Home Appliance Interface
* File Name:         Errorlog.c
********************************************************************************
* Description:
*  Provides a method for system processes to log errors.  The log can be read
*  at a later time to view the history of a failure.
********************************************************************************
*/

#include "ErrorLog.h"

uint8 ErrorLogArray[ERROR_LOG_SIZE];

ERROR_INDEX_TYPE ErrorIndex;
ERROR_INDEX_TYPE ErrorCount;

void Log_Error(uint8 ProcessID, uint8 Error)
{
    mDebugSet(System_DebugOutput, DEBUG_ERROR_LOGGED_MASK);
    mDebugSet(System_DebugOutput, DEBUG_ERROR_MASK);
    
    if(ErrorIndex < (ERROR_LOG_SIZE - 1u))
    {
        ErrorLogArray[ErrorIndex] = ProcessID;
        ErrorIndex++;
        ErrorLogArray[ErrorIndex] = Error;
        ErrorIndex++;
        
        ErrorCount++;
    }
    
    mDebugClear(System_DebugOutput, DEBUG_ERROR_LOGGED_MASK);
    
    return;
}

void ClearLog(void)
{
    ERROR_INDEX_TYPE i;
    
    for(i = 0; i < ERROR_LOG_SIZE; i++)
    {
        ErrorLogArray[i] = 0u;
    }
    
    ErrorIndex = 0u;
    ErrorCount = 0u;
    
    mClearErrorPin();
    
    return;
}

/* [] END OF FILE */
