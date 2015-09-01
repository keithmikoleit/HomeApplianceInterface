/*******************************************************************************
* Project Name:      PSoC 4 BLE Home Appliance Interface
* File Name:         SystemUtils.c
********************************************************************************
* Description:
*  Any system level utitlities should be added here.  These are items that
*  provide functionality any process could find useful.
*
********************************************************************************
*/
#include "SystemUtils.h"

void Set16ByPtr(uint8 ptr[], uint16 value)
{
    ptr[0u] = (uint8) value;
    ptr[1u] = (uint8) (value >> 8u);    
}

void Set32ByPtr(uint8 ptr[], uint32 value)
{
    ptr[0u] = (uint8) value;
    ptr[1u] = (uint8) (value >> 8u);    
    ptr[2u] = (uint8) (value >> 16u);    
    ptr[3u] = (uint8) (value >> 24u);
}

/* [] END OF FILE */
