/*******************************************************************************
* Project Name:      PSoC 4 BLE Home Appliance Interface
* File Name:         TestMux.c
********************************************************************************
* Description:
*  The test mux provides debug utility by allowing a variety of signals to be
*  output on pins.
*
*  TODO: This process must be optimized for P4 BLE as it currently takes up
*        too many digital resources.
*
********************************************************************************
*/

#include "TestMux.h"

#if (PROCESS_DEBUG_ENABLED == 1u)
    
/* define the array of pointers to pointers.  These pointers will point to each 
individual processes pointers after the processes register their piointer with the
testmux.  These pointer arrays will then be used to manipulate what each processes
debug pointer points to.  Each processes debug pointer can either point to a 
"dead end" variable, or to a specific location defined in the testmux.h header file.
This special location is usually a control register */
#if (NUMBER_OF_PROCESSES_MAX == 32u)
    uint8 ** aDebugPointer[NUMBER_OF_PROCESSES_MAX+1] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, \
                                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, \
                                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, \
                                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
#endif

#if (NUMBER_OF_PROCESSES_MAX == 16u)
    uint8 ** aDebugPointer[NUMBER_OF_PROCESSES_MAX+1] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, \
                                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
#endif

#if (NUMBER_OF_PROCESSES_MAX == 8u)
    uint8 ** aDebugPointer[NUMBER_OF_PROCESSES_MAX+1] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
#endif

/* The dead end location for all processes we dont care about */
uint8 DeadEnd;
uint8 TestMuxTest = 0;

void TestMux_Init(void)
{
    
}

uint8 TestMux_Register(uint8 ProcessID, uint8 * * DebugPointer)
{
    if((ProcessID >= NUMBER_OF_PROCESSES_MAX) && (ProcessID != SYSTEM_PROCESS_ID))
    {
        Log_Error(TESTMUX_PROCESS_ID, TESTMUX_ERROR_PROCESS_ID_OUT_OF_RANGE);
        return TESTMUX_FAIL;
    }
    
    if(aDebugPointer[ProcessID] != NULL)
    {
        Log_Error(TESTMUX_PROCESS_ID, TESTMUX_ERROR_PROCESS_ID_ALREADY_REGISTERED);
        return TESTMUX_FAIL;
    }
    
    *DebugPointer = &DeadEnd;
    aDebugPointer[ProcessID] = DebugPointer;
    
    return TESTMUX_SUCCESS;
}

uint8 TestMux_SelectProcess0(uint8 ProcessID)
{
    static uint8 PreviousProcessID = 255u;
    
    if((ProcessID >= NUMBER_OF_PROCESSES_MAX) && (ProcessID != SYSTEM_PROCESS_ID))
    {
        Log_Error(TESTMUX_PROCESS_ID, TESTMUX_ERROR_PROCESS_ID_OUT_OF_RANGE);
        return TESTMUX_FAIL;
    }
    
    if(aDebugPointer[ProcessID] == NULL)
    {
        Log_Error(TESTMUX_PROCESS_ID, TESTMUX_ERROR_PROCESS_ID_NOT_REGISTERED);
        return TESTMUX_FAIL;
    }
    
    if(*aDebugPointer[ProcessID] != &DeadEnd)
    {
        Log_Error(TESTMUX_PROCESS_ID, TESTMUX_ERROR_PROCESS_ID_ALREADY_ASSIGNED);
        return TESTMUX_FAIL;
    }
    
    if(PreviousProcessID != 255u)
    {
        *aDebugPointer[PreviousProcessID] = &DeadEnd;
    }
    
    *aDebugPointer[ProcessID] = (uint8*)&DEBUG_OUTPUT0;
    
    PreviousProcessID = ProcessID;
    
    return TESTMUX_SUCCESS;
}

uint8 TestMux_SelectProcess1(uint8 ProcessID)
{
    static uint8 PreviousProcessID = 255u;
    
    if((ProcessID >= NUMBER_OF_PROCESSES_MAX) && (ProcessID != SYSTEM_PROCESS_ID))
    {
        Log_Error(TESTMUX_PROCESS_ID, TESTMUX_ERROR_PROCESS_ID_OUT_OF_RANGE);
        return TESTMUX_FAIL;
    }
    
    if(aDebugPointer[ProcessID] == NULL)
    {
        Log_Error(TESTMUX_PROCESS_ID, TESTMUX_ERROR_PROCESS_ID_NOT_REGISTERED);
        return TESTMUX_FAIL;
    }
    
    if(*aDebugPointer[ProcessID] != &DeadEnd)
    {
        Log_Error(TESTMUX_PROCESS_ID, TESTMUX_ERROR_PROCESS_ID_ALREADY_ASSIGNED);
        return TESTMUX_FAIL;
    }
    
    if(PreviousProcessID != 255u)
    {
        *aDebugPointer[PreviousProcessID] = &DeadEnd;
    }
    
    *aDebugPointer[ProcessID] = (uint8*)&DEBUG_OUTPUT1;
    
    PreviousProcessID = ProcessID;
    
    return TESTMUX_SUCCESS;
}

uint8 TestMux_SelectSignal0(uint8 Channel)
{
    if(Channel >= MAX_HARDWARE_MUX_CHANNELS)
    {
        Log_Error(TESTMUX_PROCESS_ID, TESTMUX_ERROR_HARDWARE_CHANNEL_OUT_OF_RANGE);
        return TESTMUX_FAIL;
    }
    
    HardwareDebugMuxSelect_Control &= ~HARDWARE0_CHANNEL_MASK;
    
    HardwareDebugMuxSelect_Control |= Channel;
    
    return TESTMUX_SUCCESS;
}

uint8 TestMux_SelectSignal1(uint8 Channel)
{
    if(Channel >= MAX_HARDWARE_MUX_CHANNELS)
    {
        Log_Error(TESTMUX_PROCESS_ID, TESTMUX_ERROR_HARDWARE_CHANNEL_OUT_OF_RANGE);
        return TESTMUX_FAIL;
    }
    
    HardwareDebugMuxSelect_Control &= ~HARDWARE1_CHANNEL_MASK;
    
    HardwareDebugMuxSelect_Control |= (Channel << 4u);
    
    return TESTMUX_SUCCESS;
}

#endif
/* [] END OF FILE */
