/*******************************************************************************
* Project Name:      PSoC 4 BLE Home Appliance Interface
* File Name:         SystemUtils.h
********************************************************************************
* Description:
*  Contains defines, function prototypes, and macros for the System Utilities.
*
********************************************************************************
*/

#ifndef SYSTEM_UTILS_H
#define SYSTEM_UTILS_H
    
#include <project.h>
   
/* General Utility Defines */
#define TRUE   (1)
#define FALSE  (0)
    
/* Hardware Mutex Type define */
/***************************************
*         MUTEX STATES                 *
****************************************/
typedef enum _MUTEX_STATE
{
    UNLOCKED = 0u,
    LOCKED
}MUTEX;
    
/* Function Prototypes */
void Set16ByPtr(uint8 ptr[], uint16 value);
void Set32ByPtr(uint8 ptr[], uint32 value);    
 
#endif
/* [] END OF FILE */
