/*******************************************************************************
* Project Name:      PSoC 4 BLE Home Appliance Interface
* File Name:         Touch.c
********************************************************************************
* Description:
*  5 element slider with swipe left, swipe right, and tap gestures.
*
*  Wrist detection scans on EDA high and low measurement pins.
*
********************************************************************************
*/

#include "Touch.h"

#if (PROCESS_DEBUG_ENABLED == 1u)
    uint8 * Touch_DebugOutput;
#endif

#if(PROCESS_UART_ENABLE == 1u)
    #include "stdio.h"
#endif

uint8 Touch_Enable;
uint16 Touch_Timer_Count;
uint16 Touch_Period;
uint16 WristDetect_Timer_Count;
uint16 WristDetect_Period;
T_TOUCH_STATE s_Touch_State;

uint8 Touch_SleepCountInit = 0u;
uint8 Touch_SleepCounter8;
uint16 Touch_SleepCounter16;

/* Flag indicating the touch interrupt has fired */
uint8 TouchIntFired = 0;

/* Gesture Variables */
static uint8 CurrentCentroid = NO_TOUCH;
static uint8 Gesture = NO_GESTURE;
static uint8 position_start;
static uint8 position_end;
static uint8 position_ready;
static uint8 distance;
static uint8 active_sensor_tick;
static uint8 direction;
static uint8 lift_off;
static uint8 touch_down;
static uint8 velocity;
static uint8 large_object_count;

/* Wrist Detect Variables */
uint8 EDAWristDetectScan;
uint8 WristDetect_ModDACVal;
uint8 WristDetect_CompDACVal;
uint16 EDA_HScanResult;
uint16 EDA_LScanResult;

/* Local Function Declarations */
void ProcessGestures(void);
void GenericPreScan(uint32 port, uint32 pin);
void GenericPostScan(uint32 port, uint32 pin);

/* Initialize the Process */
void Touch_Process_Init(void)
{
    #if (PROCESS_DEBUG_ENABLED == 1u)
        if(TestMux_Register(TOUCH_PROCESS_ID, &Touch_DebugOutput)  == TESTMUX_FAIL)
        {
            Log_Error(TOUCH_PROCESS_ID, TOUCH_ERROR_FAILED_TO_REGISTER_TESTMUX);
        }
    #endif
    
    #if(PROCESS_UART_ENABLE == 1u)
        DUART_Start();
    #endif
    
    Touch_Enable = TOUCH_ENABLE_INIT;
    Touch_Timer_Count = TOUCH_IDLE_SCAN_PERIOD;
    Touch_Period = TOUCH_IDLE_SCAN_PERIOD;
    s_Touch_State = S_TOUCH_STATE_INIT;
    
    /* Initialize wrist detect parameters */
    WristDetect_ModDACVal = WRIST_DETECT_MOD_DAC;
    WristDetect_CompDACVal = WRIST_DETECT_COMP_DAC;
    WristDetect_Timer_Count = WRIST_DETECT_SCAN_PERIOD;
    WristDetect_Period = WRIST_DETECT_SCAN_PERIOD;
    
    mTouch_EnableProcess();
    
    #if(Capsense__DISABLED == 0u)
    /* Start and Initialize the Capsense component.
       InitializeAllBaselines Blocks for 1 complete scan cycle */
	CapSense_Start();
	CapSense_InitializeAllBaselines();
    #endif
    return;
}

/* Touch Process state machine */
void Touch_Process(void)
{
    mDebugSet(Touch_DebugOutput, TOUCH_DEBUG_ENTER_SM);

    #if(Capsense__DISABLED == 0u)
    
    switch(s_Touch_State)
    {
        case TOUCH_STARTSCAN:
            /************************************/
            /*    START SCAN OF TOUCH SLIDER    */
            /************************************/
            mDebugSet(Touch_DebugOutput, TOUCH_DEBUG_START_SCAN);
            /* Wakeup the CSD Hardware */
            CapSense_Wakeup();
            /* Start the hardware Scan */
            CapSense_ScanEnabledWidgets();
            /* Disable Deep Sleep while hardware scan is running */
            mTouch_DisallowDeepSleep();
            
            mTouch_SetNextState(TOUCH_IS_SCAN_COMPLETE);
            mTouch_ExecuteOnNextCoOp();
            mDebugClear(Touch_DebugOutput, TOUCH_DEBUG_START_SCAN);
        break;
            
        case TOUCH_IS_SCAN_COMPLETE:
            /************************************/
            /*    WAIT FOR SCAN TO COMPLETE     */
            /************************************/
            mDebugSet(Touch_DebugOutput, TOUCH_DEBUG_WAIT_FOR_SCAN);
            /* Check to see if the final capsense scan has run */
            if(!CapSense_IsBusy())
            {
                /* Scan is done, update associated touch data */
                /* Update Baselines */
            	CapSense_UpdateEnabledBaselines();	
                /* Check if any widget is active (this updates the SensorOn array) */
                CapSense_CheckIsAnyWidgetActive();
                /* Sleep the Capsense Hardware */
                CapSense_Sleep();
                /* Allow Deep sleep now that the hardware has finished */
                mTouch_AllowDeepSleep();
                
                /* Next we will process the results */
                mTouch_SetNextState(TOUCH_PROCESS_RESULTS);
                mTouch_ExecuteOnNextCoOp();
            }
            else
            {
                mTouch_SetNextState(TOUCH_IS_SCAN_COMPLETE);
                /* Wait for scan to be done.  Let the system sleep
                   while we wait */
                mTouch_DeQueue();
            }
            
            mDebugClear(Touch_DebugOutput, TOUCH_DEBUG_WAIT_FOR_SCAN);
        break;
        
        case TOUCH_PROCESS_RESULTS:
            /************************************/
            /*PROCESS SCAN RESULTS INTO GESTURES*/
            /************************************/
            mDebugSet(Touch_DebugOutput, TOUCH_DEBUG_PROCESS_RESULTS);
            
            /* Process Scan Results */
            ProcessGestures();
            
            /* If the wrist detect timer runs out we run a wrist detect scan.
             * we only check if its less than the Touch period so that we don't
             * miss a scan because of underflow */
            if(WristDetect_Timer_Count <=  Touch_Period)
            {
                /* Reset the timer */
                WristDetect_Timer_Count = WristDetect_Period;
                mTouch_SetNextState(WRIST_DETECT_SCAN_H);
                mTouch_ExecuteOnNextCoOp();
            }
            else
            {
                
                /* Decrement Wrist detect timer */
                /* Subtract off touch period */
                WristDetect_Timer_Count -= Touch_Period;
                /* Setup next touch scan */
                mTouch_SetNextState(TOUCH_STARTSCAN);
                /* Sleep the Capsense hardware */
                CapSense_Sleep();
                /* Allow Deep Sleep */
                mTouch_AllowDeepSleep();
                /* finished processing, dequeue */
                mTouch_DeQueue();
                
            }
            mDebugClear(Touch_DebugOutput, TOUCH_DEBUG_PROCESS_RESULTS);
        break;
        
        case WRIST_DETECT_SCAN_H:
            /************************************/
            /*    START CSD SCAN OF EDA H PIN   */
            /************************************/
            mDebugSet(Touch_DebugOutput, TOUCH_DEBUG_SCAN_EDA_H);
         
            //FirmwareDebugOutput1_Write(0x80);
            
            /* Set Flag indicating this is an EDA wrist detect scan for the
             * CSD interrupt */
            EDAWristDetectScan = TRUE;
            
            /* Wakeup the CSD Hardware */
            CapSense_Wakeup();
            
            GenericPreScan(EDA_PIN_PORT, EDA_H_PIN_SHIFT);
            
            /* Disable Deep Sleep while hardware scan is running */
            mTouch_DisallowDeepSleep();
            
            /* Wait for scan to be done.  Let the system sleep
               while we wait */
            mTouch_DeQueue();
            
            mTouch_SetNextState(WRIST_DETECT_PROCESS_H);
                
            mDebugClear(Touch_DebugOutput, TOUCH_DEBUG_SCAN_EDA_H);
            break;
            
        case WRIST_DETECT_PROCESS_H:
            /************************************/
            /*    PROCESS RESULTS OF EDA H SCAN */
            /************************************/
                mDebugSet(Touch_DebugOutput, TOUCH_DEBUG_PROCESS_EDA_H);
        
                /* Wait until the CSD scan completes */
                if(!CapSense_IsBusy())
                {
                    /* Run post scan for EDA_MEASURE_H */
                    
                    /* Read SlotResult from Raw Counter */
                    EDA_HScanResult  = (uint16)CapSense_CSD_CNT_REG;
                    
                    GenericPostScan(EDA_PIN_PORT, EDA_H_PIN_SHIFT);
                    
                    /* Setup scan for EDA_MEASURE_L on next Coop */
                    mTouch_SetNextState(WRIST_DETECT_SCAN_L);
                }
                else
                {
                    mTouch_SetNextState(WRIST_DETECT_PROCESS_H);
                    /* Wait for scan to be done.  Let the system sleep
                       while we wait */
                    mTouch_DeQueue();
                }
                
                mDebugClear(Touch_DebugOutput, TOUCH_DEBUG_PROCESS_EDA_H);
            break;
                
        case WRIST_DETECT_SCAN_L:
            /************************************/
            /*    START SCAN OF EDA L PIN       */
            /************************************/
            mDebugSet(Touch_DebugOutput, TOUCH_DEBUG_SCAN_EDA_L);
                
            /* Run pre scan for EDA_MEASURE_L */    
                
            /* Set Flag indicating this is an EDA wrist detect scan for the
             * CSD interrupt */
            EDAWristDetectScan = TRUE;
            
            GenericPreScan(EDA_PIN_PORT, EDA_L_PIN_SHIFT);    
            
            /* Disable Deep Sleep while hardware scan is running */
            mTouch_DisallowDeepSleep();
            
            /* Wait for scan to be done.  Let the system sleep
               while we wait */
            mTouch_DeQueue();
            
            mTouch_SetNextState(WRIST_DETECT_PROCESS_L);          
            
            mDebugClear(Touch_DebugOutput, TOUCH_DEBUG_SCAN_EDA_L);
            
        break;
            
        case WRIST_DETECT_PROCESS_L:
            /************************************/
            /*    PROCESS RESULTS OF EDA L SCAN */
            /************************************/
            mDebugSet(Touch_DebugOutput, TOUCH_DEBUG_PROCESS_EDA_L);
            /* Wait till the CSD scan completes */
                
            /* Run post scan for EDA_MEASURE_L */            
             if(!CapSense_IsBusy())
            {
                /* Run post scan for EDA_MEASURE_L */
                
                /* Read SlotResult from Raw Counter */
                EDA_LScanResult  = (uint16)CapSense_CSD_CNT_REG;
                
                GenericPostScan(EDA_PIN_PORT, EDA_L_PIN_SHIFT);
                
                #if(PROCESS_UART_ENABLE == 1u)
                    // Send out the resulting scan data over UART
                    uint8 buf[50];
                    uint8 len;
                    len = sprintf((char*)buf, "EDA_L: %d EDA_H: %d\r\n", EDA_LScanResult, EDA_HScanResult);
                    DUART_SpiUartPutArray(buf, len);
                #endif
                
                mTouch_SetNextState(WRIST_DETCT_FINISH_SCAN);               
            }
            else
            {
                mTouch_SetNextState(WRIST_DETECT_PROCESS_H);
                /* Wait for scan to be done.  Let the system sleep
                   while we wait */
                mTouch_DeQueue();
            }           
            mDebugClear(Touch_DebugOutput, TOUCH_DEBUG_PROCESS_EDA_L);
        break;
            
        case WRIST_DETCT_FINISH_SCAN:
            /************************************/
            /*        Shut down CSD HW          */
            /************************************/
    
            /* Return EDA Mutex */
            /* Setup scan for EDA_MEASURE_L on next Coop */
            mTouch_SetNextState(TOUCH_STARTSCAN);
            /* Sleep the Capsense hardware */
            CapSense_Sleep();
            /* Allow Deep Sleep */
            mTouch_AllowDeepSleep();
            /* finished processing, dequeue */
            mTouch_DeQueue();
            /* Clear Flag indicating this is an EDA wrist detect scan for the
             * CSD interrupt */
            EDAWristDetectScan = FALSE;
        
        break;
            
        default:
            Log_Error(TOUCH_PROCESS_ID, TOUCH_ERROR_DEFAULT_STATE);
            mTouch_SetNextState(TOUCH_STARTSCAN);
            mTouch_DeQueue();
            mTouch_DisableProcess();
        break;
    }     
    
    #endif
    
    mDebugClear(Touch_DebugOutput, TOUCH_DEBUG_ENTER_SM);
    
    return;
}

/*******************************************************************************
* Function Name: TouchInterruptFired
********************************************************************************
*
* Summary:
*  This function returns the flag indicating whether the touch interrupt
*  has fired or not.  The flag is set at the end of each scan and is cleared
*  by the touch process when it checks to see if the current set of scans
*  has completed.
*
* Parameters:
*  None.
*
* Return:
*  Touch interrupt fired flag.
*
*******************************************************************************/
uint8 TouchInterruptFired(void)
{
    return TouchIntFired;
}

/*******************************************************************************
* Function Name: GetGesture
********************************************************************************
*
* Summary:
*  This is the get function for the latest decoded gesture from the touch
*  process.
*
* Parameters:
*  None.
*
* Return:
*  Latest decoded gesture.
*
*******************************************************************************/
uint8 GetGesture(void)
{
    return Gesture;   
}

/*******************************************************************************
* Function Name: Process_Gestures
********************************************************************************
*
* Summary:
*  This function processes the gestures after each touch scan. Looks for the 
*   following gestures:
*    - Large object (all sensors active)
*    - Swipe left
*    - Swipe right
*    - Tap
*
* Parameters:
*  None.
*
* Return:
*  None.
*
*******************************************************************************/
void ProcessGestures(void)
{	
	/* Large Object Rejection: if 4 sensors are active at the same time do nothing  */
	if ((CapSense_sensorOnMask[0] & SLIDER_MASK) == SLIDER_MASK) 
	{
        /* Debounce Large Object Presence */
        /* All sensors must be active for a specified number of scans before it is considered an invalid touch */
        large_object_count++;
        if(large_object_count >= LARGE_OBJECT_DEBOUNCE)
        {
		    Gesture = LARGE_OBJECT;
            large_object_count = 0;
        }
	}
    else
    {
        large_object_count = 0;
    }
	
    /* After Release Clear INVALID_TOUCH Flag */
	if ((CapSense_sensorOnMask[0] & SLIDER_MASK) == 0)
	{
		Gesture = NO_GESTURE;  
	}
	
    /* If no current gesture (e.g. LARGE_OBJECT), then proceed with gesture detection */
	if (Gesture == NO_GESTURE)
	{
        /* Get Centroid */
		CurrentCentroid = CapSense_GetCentroidPos(CapSense_LINEARSLIDER0__LS); 
        
		/* Swipe processing */			
		if (CapSense_sensorOnMask[0u] & SLIDER_MASK) 			
		{
			if (CurrentCentroid != NO_TOUCH) 
			{
				position_ready = CurrentCentroid;		
			}
			
            /* Check to see if this is start of touch */
			if((!touch_down)&&(CurrentCentroid != NO_TOUCH)) 
			{
				position_start = position_ready;                /* Save Start Location */
                touch_down = TRUE;                              
				lift_off = FALSE;
                /* Update scan period to active period */
                Touch_Period = TOUCH_ACTIVE_SCAN_PERIOD;
			}
			
            /* Increment Touch Duration Timer */
			active_sensor_tick++;				
		}
		else
		{   
            /* No Touch */
            if(touch_down)      /* Is this first lift off after a touch? */
            {
                lift_off = TRUE;						
				touch_down = FALSE;
                /* No Current touch, move to Idle scan period */
                Touch_Period = TOUCH_IDLE_SCAN_PERIOD;

    			position_end = position_ready;                  /* Save End Location */
    			
                /* Swipe Right Direction Check */
    			if (position_end > position_start)					
    			{
    				distance = position_end - position_start;
    				direction = DIRECTION_LEFT;						
    			}
                /* Swipe Left Direction Check */
    			else												
    			{
    				distance = position_start - position_end;
    				direction = DIRECTION_RIGHT;
    			}	
                
                /* Calculate Velocity (Not currently used) */
    			velocity = distance / active_sensor_tick;
                
                /* SWIPE: Check to see if swipe distance and timing criteria are met */
    			if ((distance > MIN_SWIPE_DISTANCE) && (active_sensor_tick >= MIN_SWIPE_TIMEOUT) && (active_sensor_tick < MAX_SWIPE_TIMEOUT))
    			{
    				if(direction)
                    {
                        Gesture = SWIPE_LEFT_GESTURE;
                    }
                    else
                    {
                        Gesture = SWIPE_RIGHT_GESTURE;
                    }
                    
    			}
                /* TAP: Check to see if tap distance and timing criteria are met */
                else if ((distance < MAX_TAP_DISTANCE) && (active_sensor_tick >= MIN_TAP_TIMEOUT) && (active_sensor_tick < MAX_TAP_TIMEOUT))
                {
                    Gesture = TAP_GESTURE;
                }
                /* Otherwise, No Gesture */
                else
                {
                    Gesture = NO_GESTURE;
                }
			}
            /* Clear Touch Tick Timer */
			active_sensor_tick = 0u;				
		}
	}
	else
	{
        /* An Invalid Touch Occurred, Clear Variables */
		CurrentCentroid = NO_TOUCH;
		direction = 0u;
		distance = 0u;
        velocity = 0u;
		active_sensor_tick = 0u;
	}
}

/*******************************************************************************
* Function Name: GenericPreScan
********************************************************************************
*
* Summary:
*  This function generalizes the Capsense component prescan for use with any
*  pin.
*
* Parameters:
*  uint8 port : Pin port to be scanned
*  uint8 pin  : Pin number to be scanned
*
* Return:
*  None.
*
*******************************************************************************/
void GenericPreScan(uint32 port, uint32 pin)
{
    CapSense_csdStatusVar = (CapSense_SW_STS_BUSY | CapSense_SW_CTRL_SINGLE_SCAN);
    //CapSense_PreScan(0);
    
    /* Run Prescan procuedure for EDA_MEASURE_H */
    uint8 interruptState;
	uint32 newRegValue;
	uint32 counterResolution;
    /* For debuggin with P2_1 */
    #ifdef USE_P2_1
	#if ((CapSense_TUNING_METHOD == CapSense__TUNING_AUTO) &&\
		 (0 != CapSense_IS_OVERSAMPLING_EN))
		uint32 oversamplingFactor;
	#endif /* ((CapSense_TUNING_METHOD == CapSense__TUNING_AUTO) &&\
	           (0 != CapSense_IS_OVERSAMPLING_EN)) */
	#endif
	/* Recalculate Counter Resolution to MSB 16 bits */
	counterResolution = CapSense_RESOLUTION_12_BITS;
            
    /* Stop sense and samp,le clocks to reset and sync */
	CapSense_SenseClk_Stop();
	CapSense_SampleClk_Stop();

    /* Set the clock divider values.  For wrist detect we are using
     * the starting calibration values.  This could be tuned further
     * to gain better performance */
	CapSense_SampleClk_SetDividerValue(CapSense_CALIBRATION_MD);
	CapSense_SenseClk_SetDividerValue(CapSense_CALIBRATION_ASD);

	/* Start up the clocks */
	CapSense_SampleClk_Start();
	CapSense_SenseClk_StartEx(CapSense_SampleClk__DIV_ID);
		
    /* Clear the counter value by running until completion.
     * clear the pending interrupt generated by running the counter
     * to completion */
	CyIntDisable(CapSense_ISR_NUMBER);
	CapSense_CSD_CNT_REG = CapSense_ONE_CYCLE;
	while(0u != (CapSense_CSD_CNT_REG & CapSense_RESOLUTION_16_BITS))
	{
	/* Wait until scanning is complete */ 
	}
	CapSense_CSD_INTR_REG = 1u;
	CyIntClearPending(CapSense_ISR_NUMBER);
	CyIntEnable(CapSense_ISR_NUMBER); 

	/* Set Idac Value */
	CyIntDisable(CapSense_ISR_NUMBER);
    newRegValue = CapSense_CSD_IDAC_REG;
 
	newRegValue &= ~(CapSense_CSD_IDAC1_DATA_MASK | CapSense_CSD_IDAC2_DATA_MASK);
    newRegValue |= (WristDetect_ModDACVal | 
					(uint32)((uint32)WristDetect_CompDACVal <<
					CapSense_CSD_IDAC2_DATA_OFFSET));                              
	
	CapSense_CSD_IDAC_REG = newRegValue;

    /* Disconnect Vref Buffer from AMUX */
	newRegValue = CapSense_CSD_CFG_REG;
	newRegValue &= ~(CapSense_PRECHARGE_CONFIG_MASK);
	newRegValue |= CapSense_CTANK_PRECHARGE_CONFIG;
	
	CyIntEnable(CapSense_ISR_NUMBER);
	
    /*  Configures the selected sensor to measure during the next measurement cycle.
     *  The corresponding pins are set to Analog High-Z mode and connected to the
     *  Analog Mux Bus. This also enables the comparator function.
    */
	uint8  pinModeShift;
    uint8  pinHSIOMShift;
	uint32 newRegisterValue;
//    uint32 port;
//	
//	port = (uint32) EDA_PIN_PORT;
//    pinModeShift = EDA_H_PIN_SHIFT  * CapSense_PC_PIN_CFG_SIZE;
//    pinHSIOMShift = EDA_H_PIN_SHIFT * CapSense_HSIOM_PIN_CFG_SIZE;
    pinModeShift = pin  * CapSense_PC_PIN_CFG_SIZE;
    pinHSIOMShift = pin * CapSense_HSIOM_PIN_CFG_SIZE;
    
    /* KLMZ DEBUG with P2_1 */

	interruptState = CyEnterCriticalSection();
	
	newRegisterValue = *CapSense_prtSelTbl[port];
	newRegisterValue &= ~(CapSense_CSD_HSIOM_MASK << pinHSIOMShift);
	newRegisterValue |= (uint32)((uint32)CapSense_CSD_SENSE_PORT_MODE << pinHSIOMShift);
   
    *CapSense_prtCfgTbl[port] &= (uint32)~((uint32)CapSense_CSD_PIN_MODE_MASK << pinModeShift);
    *CapSense_prtSelTbl[port] = newRegisterValue;
    
    /* We also need to connect the port 2 side of the AMUX to the
     * port 1 side of the amux.  This is not done automatically by Creator
     * because the slider pins are on port and and the other side of the mux
     * is not needed.
     * Note that nothing else can be connected to or require use of the AMUX
     * while this scan is running.  This includes static pin connections.
    */
    *((reg32 *)CYREG_HSIOM_AMUX_SPLIT_CTL2) |= CONNECT_AMUXL_AMUXR;
	
	CyExitCriticalSection(interruptState);
    
	interruptState = CyEnterCriticalSection();
	CapSense_CSD_CFG_REG = newRegValue;
	
	/* `#START CapSense_PreSettlingDelay_Debug` */

	/* `#END` */
	
	CyDelayCycles(CapSense_GLITCH_ELIMINATION_CYCLES);
	
	/* `#START CapSense_PreScan_Debug` */

	/* `#END` */
	
    CapSense_CSD_CNT_REG = counterResolution;
    CyExitCriticalSection(interruptState);       
}

/*******************************************************************************
* Function Name: GenericPostScan
********************************************************************************
*
* Summary:
*  This function generalizes the Capsense component postscan for use with any
*  pin.
*
* Parameters:
*  uint8 port : Pin port to be scanned
*  uint8 pin  : Pin number to be scanned
*
* Return:
*  None.
*
*******************************************************************************/
void GenericPostScan(uint32 port, uint32 pin)
{
    CyIntDisable(CapSense_ISR_NUMBER);
                    


    /* Disable Sensor */
    /*  Disables the selected sensor. The corresponding pin is disconnected from the
     *  Analog Mux Bus and connected High_Z */
    uint8 interruptState;
	uint32 newRegisterValue;
    uint32 newRegValue;
    uint32 inactiveConnect = CapSense_SNS_HIZANALOG_CONNECT;
    	
    uint8  pinModeShift;
    uint8  pinHSIOMShift;
    //uint32 port;

	//port = (uint32) EDA_PIN_PORT;
    pinModeShift = pin  * CapSense_PC_PIN_CFG_SIZE;
    pinHSIOMShift = pin * CapSense_HSIOM_PIN_CFG_SIZE;

    /* Disconnect the pin from the AMUX bus */
    *((reg32 *) CYREG_HSIOM_PORT_SEL1) &= ~(CapSense_CSD_HSIOM_MASK << pinHSIOMShift);
    /* We also disconnect the two sides of the AMUX here */
    *((reg32 *)CYREG_HSIOM_AMUX_SPLIT_CTL2) &= ~CONNECT_AMUXL_AMUXR;
    
    interruptState = CyEnterCriticalSection();

    /* Set drive mode */
	newRegisterValue = *CapSense_prtCfgTbl[port];
	newRegisterValue &= ~(CapSense_CSD_PIN_MODE_MASK << pinModeShift);
	newRegisterValue |=  (uint32)(inactiveConnect << pinModeShift);
    *CapSense_prtCfgTbl[port] =  newRegisterValue;

    CyExitCriticalSection(interruptState);
	
	/* Connect Vref Buffer to AMUX bus  */
	newRegValue = CapSense_CSD_CFG_REG;
	newRegValue &= ~(CapSense_PRECHARGE_CONFIG_MASK);
	newRegValue |= CapSense_CMOD_PRECHARGE_CONFIG;
	CapSense_CSD_CFG_REG = newRegValue;

    	/* Set Idac Value = 0 */
    #if (CapSense_IDAC_CNT == 1u)   
    	CapSense_CSD_IDAC_REG &= ~(CapSense_CSD_IDAC1_DATA_MASK);
    #else
    	CapSense_CSD_IDAC_REG &= ~(CapSense_CSD_IDAC1_DATA_MASK | CapSense_CSD_IDAC2_DATA_MASK);                               
    #endif /* (CapSense_IDAC_CNT == 1u) */

    CyIntEnable(CapSense_ISR_NUMBER);
}

/* [] END OF FILE */
