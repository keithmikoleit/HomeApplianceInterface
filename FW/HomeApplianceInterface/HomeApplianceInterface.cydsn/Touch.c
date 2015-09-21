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
T_TOUCH_STATE s_Touch_State;

uint8 Touch_SleepCountInit = 0u;
uint8 Touch_SleepCounter8;
uint16 Touch_SleepCounter16;

/* Gesture Variables */
Touch_Output TouchResult;
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

/* Local Function Declarations */
void ProcessGestures(void);

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
    
    mTouch_EnableProcess();
    
    #if(Capsense__DISABLED == 0u)
    /* Start and Initialize the Capsense component.
       InitializeAllBaselines Blocks for 1 complete scan cycle */
	CapSense_Start();
	CapSense_InitializeAllBaselines();
    #endif
    
    /* Initialize BLE data packet */
    TouchResult.CurrentCentroid = NO_TOUCH;
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
            
            /* Let BLE know data is ready */
            TouchResult.Data_Ready = true;
            
            /* Setup next touch scan */
            mTouch_SetNextState(TOUCH_STARTSCAN);
            /* finished processing, dequeue */
            mTouch_DeQueue();

            mDebugClear(Touch_DebugOutput, TOUCH_DEBUG_PROCESS_RESULTS);
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
		TouchResult.CurrentCentroid = CapSense_GetCentroidPos(CapSense_LINEARSLIDER0__LS); 
        
		/* Swipe processing */			
		if (CapSense_sensorOnMask[0u] & SLIDER_MASK) 			
		{
			if (TouchResult.CurrentCentroid != NO_TOUCH) 
			{
				position_ready = TouchResult.CurrentCentroid;		
			}
			
            /* Check to see if this is start of touch */
			if((!touch_down)&&(TouchResult.CurrentCentroid != NO_TOUCH)) 
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
		TouchResult.CurrentCentroid = NO_TOUCH;
		direction = 0u;
		distance = 0u;
        velocity = 0u;
		active_sensor_tick = 0u;
	}
}

/* [] END OF FILE */
