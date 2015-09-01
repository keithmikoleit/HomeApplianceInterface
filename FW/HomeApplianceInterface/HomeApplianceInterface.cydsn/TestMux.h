/*******************************************************************************
* Project Name:      PSoC 4 BLE Home Appliance Interface
* File Name:         TestMux.h
********************************************************************************
* Description:
*  Contains defines, function prototypes, and macros for the Test Mux.
*
********************************************************************************
*/

#ifndef TESTMUX_HEADER
#define TESTMUX_HEADER
    
#include "main.h"
    
#define DEBUG_OUTPUT0               (FirmwareDebugOutput0_Control)
#define DEBUG_OUTPUT1               (FirmwareDebugOutput1_Control)
#define TEST_MUX_TEST               (TestMuxTest)
    
#define TESTMUX_SUCCESS             (0u)
#define TESTMUX_FAIL                (0xFFu)

void TestMux_Init(void);
uint8 TestMux_Register(uint8 ProcessID, uint8 * * DebugPointer);
uint8 TestMux_SelectProcess0(uint8 ProcessID);
uint8 TestMux_SelectProcess1(uint8 ProcessID);
uint8 TestMux_SelectSignal0(uint8 Channel);
uint8 TestMux_SelectSignal1(uint8 Channel);

/* Error definitions */
#define TESTMUX_ERROR_PROCESS_ID_OUT_OF_RANGE           (0u)
#define TESTMUX_ERROR_PROCESS_ID_ALREADY_REGISTERED     (1u)
#define TESTMUX_ERROR_PROCESS_ID_NOT_REGISTERED         (2u)
#define TESTMUX_ERROR_PROCESS_ID_ALREADY_ASSIGNED       (3u)
#define TESTMUX_ERROR_HARDWARE_CHANNEL_OUT_OF_RANGE     (4u)

#define MAX_HARDWARE_MUX_CHANNELS                       (16u)

/* hardware mux channel definitions */
#define HARDWARE0_CHANNEL_MASK                          (0x0F)
#define HARDWARE1_CHANNEL_MASK                          (0xF0)
#define HARDWARE_CHANNEL0                               (0x00)
#define HARDWARE_CHANNEL1                               (0x01)
#define HARDWARE_CHANNEL2                               (0x02)
#define HARDWARE_CHANNEL3                               (0x03)
#define HARDWARE_CHANNEL4                               (0x04)
#define HARDWARE_CHANNEL5                               (0x05)
#define HARDWARE_CHANNEL6                               (0x06)
#define HARDWARE_CHANNEL7                               (0x07)
#define HARDWARE_CHANNEL8                               (0x08)
#define HARDWARE_CHANNEL9                               (0x09)
#define HARDWARE_CHANNEL10                              (0x0A)
#define HARDWARE_CHANNEL11                              (0x0B)
#define HARDWARE_CHANNEL12                              (0x0C)
#define HARDWARE_CHANNEL13                              (0x0D)
#define HARDWARE_CHANNEL14                              (0x0E)
#define HARDWARE_CHANNEL15                              (0x0F)
    
/* macros */
#if (PROCESS_DEBUG_ENABLED == 1u)
    #define mDebugSet(DEBUG_VARIABLE, DEBUG_MASK) \
        do\
        {\
            * DEBUG_VARIABLE |= DEBUG_MASK;\
            TEST_MUX_TEST |= DEBUG_MASK;\
        } while(0)
#else
    #define mDebugSet(DEBUG_VARIABLE, DEBUG_MASK) \
        do\
        {\
        } while(0)
#endif

#if (PROCESS_DEBUG_ENABLED == 1u)
    #define mDebugClear(DEBUG_VARIABLE, DEBUG_MASK) \
        do\
        {\
            * DEBUG_VARIABLE &= ~DEBUG_MASK;\
            TEST_MUX_TEST &= ~DEBUG_MASK;\
        } while(0)
#else
    #define mDebugClear(DEBUG_VARIABLE, DEBUG_MASK) \
        do\
        {\
        } while(0)
#endif

#if (PROCESS_DEBUG_ENABLED == 1u)
    #define mDebugToggle(DEBUG_VARIABLE, DEBUG_MASK) \
        do\
        {\
            * DEBUG_VARIABLE ^= DEBUG_MASK;\
        } while(0)
#else
    #define mDebugToggle(DEBUG_VARIABLE, DEBUG_MASK) \
        do\
        {\
        } while(0)
#endif

#if (PROCESS_DEBUG_ENABLED == 1u)
    #define mDebugPulse(DEBUG_VARIABLE, DEBUG_MASK) \
        do\
        {\
            * DEBUG_VARIABLE |= DEBUG_MASK;\
            * DEBUG_VARIABLE &= ~DEBUG_MASK;\
        } while(0)
#else
    #define mDebugPulse(DEBUG_VARIABLE, DEBUG_MASK) \
        do\
        {\
        } while(0)
#endif

#endif

extern uint8 TestMuxTest;

/* [] END OF FILE */
