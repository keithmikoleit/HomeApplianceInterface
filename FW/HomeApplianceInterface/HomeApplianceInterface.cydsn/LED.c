/*******************************************************************************
* Project Name:      PSoC 4 BLE Home Appliance Interface
* File Name:         LED.c
* Dependency:        CY8CKIT-042 BLE Pioneer Kit
********************************************************************************
* Description:
*  The LED process is a debug process used to check the functionality of other
*  block procceses.  The LED process currently supports updates from IAS alerts
*  or from Touch slider results.
*
********************************************************************************
*/

#include "LED.h"
#include "BLE.h"

#if (PROCESS_DEBUG_ENABLED == 1u)
    uint8 * LED_DebugOutput;
#endif

uint8 LED_Enable = LED_ENABLE_INIT;
uint16 LED_Timer_Count = LED_PROCESS_PERIOD_INIT;
uint16 LED_Period = LED_PROCESS_PERIOD_INIT;
T_LED_STATE s_LED_State = S_LED_STATE_INIT;

uint8 LED_SleepCountInit = 0u;
uint8 LED_SleepCounter8;
uint16 LED_SleepCounter16;

/* used for testing CSD slider */
uint16 SliderPosition;
uint8 EDA_Result;

/* Variables used for updating LEDs based on Touch Gestures */
static uint8 Gesture;
static uint32 LED_Timer;

/* Initialize the Process */
void LED_Process_Init(void)
{
    #if (PROCESS_DEBUG_ENABLED == 1u)
        if(TestMux_Register(LED_PROCESS_ID, &LED_DebugOutput)  == TESTMUX_FAIL)
        {
            Log_Error(LED_PROCESS_ID, LED_ERROR_FAILED_TO_REGISTER_TESTMUX);
        }
    #endif
    
    /* Run the LED process everytime through the loop */
    LED_Period = 1;
    LED_Timer_Count = 1;
    mLED_EnableProcess();
    mLED_SetNextState(LED_STATE_1);
    
    /* Turn off RGB LED */
    BLUE_P3_7_Write(1);
    GREEN_P3_6_Write(1);
    RED_P2_6_Write(1);
    
    //mLED_DisallowSleep();
    //mLED_DisallowAltActive();
    return;
}

/* LED Process state machine */
void LED_Process(void)
{
    mDebugSet(LED_DebugOutput, LED_DEBUG_ENTER_SM);

    #if(LED_UPDATE_SOURCE == UPDATE_FROM_IAS_ALERT)
    
    /* Update Alert LED status based on IAS Alert level characteristic. */
    switch(GetAlertLevel())
    {
        case NO_ALERT:
            BLUE_P3_7_Write(1);
            GREEN_P3_6_Write(1);
            RED_P2_6_Write(1);
            break;

        case MILD_ALERT:
            BLUE_P3_7_Write(0);
            GREEN_P3_6_Write(1);
            RED_P2_6_Write(1);
            break;
            
        case HIGH_ALERT:
            BLUE_P3_7_Write(1);
            GREEN_P3_6_Write(1);
            RED_P2_6_Write(0);
            break;
        default:
            BLUE_P3_7_Write(1);
            GREEN_P3_6_Write(1);
            RED_P2_6_Write(1);
            break;
    }
    
    #elif (LED_UPDATE_SOURCE == UPDATE_FROM_TOUCH)
    
    /* Update LEDs based on Touch Gesture */
    Gesture = GetGesture();
    switch(Gesture)
    {
        case(TAP_GESTURE):
            BLUE_P3_7_Write(LED_ON); 
            LED_Timer = LED_ON_TIME_MS;
        break;
        case(SWIPE_LEFT_GESTURE):
            GREEN_P3_6_Write(LED_ON);
            LED_Timer = LED_ON_TIME_MS;
        break;
        case(SWIPE_RIGHT_GESTURE):
            RED_P2_6_Write(LED_ON);
            LED_Timer = LED_ON_TIME_MS;
        break;
        case(LARGE_OBJECT):
            RED_P2_6_Write(LED_ON);
            GREEN_P3_6_Write(LED_ON);
            BLUE_P3_7_Write(LED_ON); 
            LED_Timer = LED_ON_TIME_MS;
        break;
        case(NO_GESTURE):
            
        break;
        default:
            /* Should Not Get Here */
            Gesture = NO_GESTURE;
        break;
    }
    /* If Timer is running, decrement timer by the current tick period */
    if(LED_Timer)
    {
        LED_Timer -= SYSTEM_TICK_TIME_MS;
    }
    /* If Timer Expires, turn off LEDs */
    if(LED_Timer <= SYSTEM_TICK_TIME_MS)
    {
        RED_P2_6_Write(LED_OFF);
        GREEN_P3_6_Write(LED_OFF);
        BLUE_P3_7_Write(LED_OFF);
        /* Set Timer to 0 until next gesture is detected */
        LED_Timer = 0;
    }
      
    #elif (LED_UPDATE_SOURCE == UPDATE_FROM_EDA)   
        
    /* Update LEDs based on EDA result */
    /* allow BLE to get the result first */
    if(IsHrResultAvailable() == false)
    {
        EDA_Result = GetEDAResult();

        if(EDA_Result > 200)
        {
            /* Light up green */
            BLUE_P3_7_Write(1);
            GREEN_P3_6_Write(0);
            RED_P2_6_Write(1);
        }
        else if(EDA_Result > 120)
        {
            /* Light up blue */
            BLUE_P3_7_Write(0);
            GREEN_P3_6_Write(1);
            RED_P2_6_Write(1);
        }
        else if(EDA_Result > 40)
        {
            /* Light up red */
            BLUE_P3_7_Write(1);
            GREEN_P3_6_Write(1);
            RED_P2_6_Write(0);
        }
        else
        {
            /* Turn off the LEDs */
            BLUE_P3_7_Write(1);
            GREEN_P3_6_Write(1);
            RED_P2_6_Write(1);
        }
    }
    
    #endif    
        
    mLED_DeQueue();
    
    mDebugClear(LED_DebugOutput, LED_DEBUG_ENTER_SM);
    
    return;
}
/* [] END OF FILE */
