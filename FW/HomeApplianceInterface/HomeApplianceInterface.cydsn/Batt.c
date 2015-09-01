/*******************************************************************************
* Project Name:      PSoC 4 BLE Home Appliance Interface
* File Name:         Batt.c
********************************************************************************
* Description:
*  Battery voltage measurement.
*
********************************************************************************
*/

#include "Batt.h"

#if (PROCESS_DEBUG_ENABLED == 1u)
    uint8 * Batt_DebugOutput;
#endif

uint8 Batt_Enable = BATT_ENABLE_INIT;
uint16 Batt_Timer_Count = BATT_PROCESS_PERIOD_INIT;
uint16 Batt_Period = BATT_PROCESS_PERIOD_INIT;
T_BATT_STATE s_Batt_State = S_BATT_STATE_INIT;

uint8 Batt_SleepCountInit = 0u;
uint8 Batt_SleepCounter8;
uint16 Batt_SleepCounter16;

int32 ADC_Battery_Result;

extern MUTEX ADC_Mutex;

/* Process Functionality Variables */
Batt_Output BattResult;

/* Initialize the Process */
void Batt_Process_Init(void)
{
    #if (PROCESS_DEBUG_ENABLED == 1u)
        if(TestMux_Register(BATT_PROCESS_ID, &Batt_DebugOutput)  == TESTMUX_FAIL)
        {
            Log_Error(BATT_PROCESS_ID, BATT_ERROR_FAILED_TO_REGISTER_TESTMUX);
        }
    #endif
    
    /* Initialize Global Battery Variable */
    BattResult.Batt_Level = 100;
    BattResult.Data_Ready = false;
    
    /* Set up the process */
    Batt_Enable = BATT_ENABLE_INIT;
    Batt_Timer_Count = BATT_PROCESS_PERIOD_INIT;
    Batt_Period = BATT_PROCESS_PERIOD_INIT;
    s_Batt_State = S_BATT_STATE_INIT;
    
    return;
}

/* Batt Process state machine */
void Batt_Process(void)
{
    static uint32 Batt_Settling_Count = 0;
    uint32 tempReg;
    mDebugSet(Batt_DebugOutput, BATT_DEBUG_ENTER_SM);

    switch(s_Batt_State)
    {
        case BATT_STATE_START:
            /******************/
            /* WAIT TO SETTLE */
            /******************/
            if(Batt_Settling_Count == 0)
            {
                /* Turn on Switch */
                Batt_SwitchControl_P0_3_Write(1);
                Batt_Settling_Count++;
                mBatt_NextTick();
            }
            else
            {
                if(Batt_Settling_Count == BATT_SETTLING_TICKS)
                {
                    /* Settling done, reset count and go to next state */
                    Batt_Settling_Count = 0;
                    mBatt_SetNextState(BATT_STATE_WAIT_ANALOG);
                }
                else
                {
                    Batt_Settling_Count++;
                    mBatt_NextTick();
                }
            }
            
        break;
        
        case BATT_STATE_WAIT_ANALOG:
            /****************************/
            /*    WAIT FOR ADC MUTEX    */
            /****************************/
            if(ADC_Mutex == UNLOCKED)
            {              
                /* Lock ADC*/
                ADC_Mutex = LOCKED;

                /* Go to Next State */
                mBatt_SetNextState(BATT_STATE_START_ADC);
            }
            else
            {
                mBatt_ExecuteOnNextCoOp();
            }

        break;
        
        case BATT_STATE_START_ADC:
            /***************************/
            /*    START/CONFIG ADC     */
            /***************************/

            /* Configure ADC for Battery Measurement */
            tempReg = ADC_SAR_CTRL_REG;
           
            /* Clear Vref, Bypass Cap and Vneg ADC bits */
            tempReg &= ~(ADC_SAR_VREF_MASK|ADC_BYPASS_EN_MASK|ADC_SAR_VNEG_MASK);
            
            /* Set 1.024V vRef, and Vssa Negative Connection */            
            tempReg |= (ADC_VREF_INTERNAL1024|ADC_NEG_VSSA);
            ADC_SAR_CTRL_REG = tempReg;

            /* Select the first EDAo measurement */
            ADC_Amux_Select(ADC_BATT_CHAN0); 
   
            /* Start an ADC conversion */
            ADC_Start();
            ADC_StartConvert();
            
            /* Disable Sleep Deep Sleep while an ADC conversion is running */
            mBatt_DisallowDeepSleep();
            
            
            mBatt_SetNextState(BATT_STATE_WAIT_ADC);
        break;
        
        case BATT_STATE_WAIT_ADC:
            /***************************/
            /* WAIT FOR ADC CONVERSION */
            /***************************/
            if(ADC_IsEndConversion(ADC_RETURN_STATUS))
            {
                ADC_Battery_Result = (int32)ADC_GetResult16(0);
                mBatt_SetNextState(BATT_STATE_FREE_ANALOG);
                mBatt_ExecuteOnNextCoOp();
            }
            else
            {
                mBatt_ExecuteOnNextCoOp();   
            }

        break;
        
        case BATT_STATE_FREE_ANALOG:
            /***************************/
            /*     RESTORE & FREE ADC  */
            /***************************/
            ADC_SAR_CTRL_REG = ADC_DEFAULT_CTRL_REG_CFG;
            ADC_Stop();
            mBatt_AllowDeepSleep();
            Batt_SwitchControl_P0_3_Write(0);
            
            /* Unlock ADC*/
            ADC_Mutex = UNLOCKED;
            mBatt_SetNextState(BATT_STATE_POST_PROCESS);
        break;
    
        case BATT_STATE_POST_PROCESS:
            /****************************/
            /* CALCULATE OUTPUT VOLTAGE */
            /****************************/
            ADC_Battery_Result *= RESISTOR_SCALE;
            ADC_Battery_Result /= ADC_DEFAULT_HIGH_LIMIT;
            
            /* Check for saturation at 100% battery level */
            if(ADC_Battery_Result <= (int32)MAX_BATTERY_VOLTAGE_MV)
            {
                /* Convert to a percentage between 3.3 V (100%) and
                   2.0 V (0 %) */
                BattResult.Batt_Level = ((ADC_Battery_Result - MIN_BATTERY_VOLTAGE_MV)*100) \
                                         / (MAX_BATTERY_VOLTAGE_MV - MIN_BATTERY_VOLTAGE_MV);
            }
            else
            {
                BattResult.Batt_Level = 100;
            }
            BattResult.Data_Ready = true;
                   
            mBatt_DeQueue();
            mBatt_SetNextState(BATT_STATE_START);
        break;
        
        default:
            Log_Error(BATT_PROCESS_ID, BATT_ERROR_DEFAULT_STATE);
            mBatt_SetNextState(BATT_STATE_START);
            mBatt_DeQueue();
            mBatt_DisableProcess();
        break;
    }       
    
    mDebugClear(Batt_DebugOutput, BATT_DEBUG_ENTER_SM);
    
    return;
}
/* [] END OF FILE */
