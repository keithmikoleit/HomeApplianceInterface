/*******************************************************************************
* Project Name:      PSoC 4 BLE Home Appliance Interface
* File Name:         BLE.c
********************************************************************************
* Description:
*  The BLE process is responsbile for initializing the BLE hardware and
*  call back functions.  Once the system is up and running the BLE process
*  is responsible for processing BLE communication events.
*
*  Because BLE is extremely time sensitive this process runs everytime we enter
*  the coop loop and does not follow the normal periodic block based process
*  structure.
*
*********************************************************************************
*/
#include "BLE.h"

#if (PROCESS_DEBUG_ENABLED == 1u)
    uint8 * BLE_DebugOutput;
#endif

uint8 BLE_Enable = BLE_ENABLE_INIT;
/* BLE follows its own path and does not utilized these system variables */
//uint16 BLE_Timer_Count = BLE_PROCESS_PERIOD_INIT;
//uint16 BLE_Period = BLE_PROCESS_PERIOD_INIT;
//T_BLE_STATE s_BLE_State = S_BLE_STATE_INIT;

uint8 BLE_SleepCountInit = 0u;
uint8 BLE_SleepCounter8;
uint16 BLE_SleepCounter16;

uint8 Device_Connected = false;

CYBLE_GATTS_HANDLE_VALUE_NTF_T notificationHandle;

/* Notification Flags */
uint8 Batt_Notification;
uint8 Touch_Notification;

/* This flag is used to let application update the CCCD value for correct read 
* operation by connected Central device */
uint8 Update_Batt_Notification = false;
uint8 Update_Touch_Notification = false;

/***************************************
*   Local Function Prototypes
***************************************/
void Stack_Event_Handler(uint32 event, void* eventParam);
void BAS_Event_Handler(uint32 event, void* eventParam);
void HTS_Event_Handler(uint32 event, void *eventParam);
void HrsEventHandler(uint32 event, void* eventParam);
void RSCS_Event_Handler(uint32 event, void *eventParam);

void Check_For_BLE_Data(void);
void Update_Gatts_Attribute(CYBLE_GATT_DB_ATTR_HANDLE_T handle, uint8* data, uint8 length);
void Send_BAS_Over_BLE(void);
void Send_Touch_Over_BLE(void);

/***************************************
*   Interal Varaibles
***************************************/

/* Initialize the Process */
void BLE_Process_Init(void)
{
    /* Create Firmware Version string from #defines in main.h */
    uint8 versionString[5] = {(uint8)'v', 
                              (uint8)ProjectMajorVersion + 0x30, 
                              (uint8)'.',
                              (uint8)ProjectMinorVersion/10 + 0x30, 
                              (uint8)ProjectMinorVersion%10 + 0x30};
    
    
    #if (PROCESS_DEBUG_ENABLED == 1u)
        if(TestMux_Register(BLE_PROCESS_ID, &BLE_DebugOutput)  == TESTMUX_FAIL)
        {
            Log_Error(BLE_PROCESS_ID, BLE_ERROR_FAILED_TO_REGISTER_TESTMUX);
        }
    #endif
    
    /* Start the BLE component and register the event handlers */
    CyBle_Start(Stack_Event_Handler);
    CyBle_BasRegisterAttrCallback(BAS_Event_Handler);
    
    /* Update Database with Current Firmware Version string */
    CyBle_DissSetCharacteristicValue(CYBLE_DIS_FIRMWARE_REV, 5, versionString);
    
    return;
}

/* BLE Process state machine */
void BLE_Process(void)
{
    mDebugSet(BLE_DebugOutput, BLE_DEBUG_ENTER_SM);

    /* Process all the pending BLE tasks. This single API call to 
    * will service all the BLE stack events. This API MUST be called at least once
    * in a BLE connection interval */
    CyBle_ProcessEvents();
    
    /* Call BLE Output Functions */
    Send_BAS_Over_BLE();
    Send_Touch_Over_BLE();
    
    /* Check for new written data from central */
    Check_For_BLE_Data();
       
    mBLE_DeQueue();
    
    mDebugClear(BLE_DebugOutput, BLE_DEBUG_ENTER_SM);
    
    return;
}

/*******************************************************************************
* Function Name: Stack_Event_Handler
********************************************************************************
*
* Summary:
*  This is an event callback function to receive events from the BLE Component.
*
* Parameters:  
*  uint8 event:       Event from the CYBLE component
*  void* eventParams: A structure instance for corresponding event type. The 
*                     list of event structure is described in the component 
*                     datasheet.
*
* Return: 
*  None
*
*******************************************************************************/
void Stack_Event_Handler(uint32 event, void *eventParam)
{
    /* Structure to store data written by Client */	
	CYBLE_GATTS_WRITE_REQ_PARAM_T *wrReqParam;
    switch(event)
    {
        /* STACK ON or Disconnect starts an Advertisement */
        case CYBLE_EVT_STACK_ON:
        case CYBLE_EVT_GAP_DEVICE_DISCONNECTED:
			/* This event is generated at GAP disconnection. 
			* Restart advertisement */
			CyBle_GappStartAdvertisement(CYBLE_ADVERTISING_FAST);
			break;
                    /* BLE stack is on. Start BLE advertisement */
			CyBle_GappStartAdvertisement(CYBLE_ADVERTISING_FAST);
            
        case CYBLE_EVT_GAPP_ADVERTISEMENT_START_STOP:
            /* This event is generated whenever Advertisement starts or stops.
			* The exact state of advertisement is obtained by CyBle_State() */
			if(CyBle_GetState() == CYBLE_STATE_DISCONNECTED)
			{
				CyBle_GappStartAdvertisement(CYBLE_ADVERTISING_FAST);
                Device_Connected = false;
			}
            break;
        
        case CYBLE_EVT_GATTS_WRITE_REQ: 
			/* This event is generated when the connected Central device sends a
			* Write request. The parameter contains the data written */
			
			/* Extract the Write data sent by Client */
            wrReqParam = (CYBLE_GATTS_WRITE_REQ_PARAM_T *) eventParam;
            
            /* Touch Notification Change */
            if(CYBLE_TOUCH_SLIDER_CURRENT_CENTROID_CLIENT_CHARACTERISTIC_CONFIGURATION_DESC_HANDLE == wrReqParam->handleValPair.attrHandle)
            {
                Touch_Notification = wrReqParam->handleValPair.value.val[CYBLE_TOUCH_SLIDER_CURRENT_CENTROID_CLIENT_CHARACTERISTIC_CONFIGURATION_DESC_INDEX];
                Update_Touch_Notification = true;
            }
			
			/* Send the response to the write request received. */
			CyBle_GattsWriteRsp(cyBle_connHandle);
			
			break;    
        
        case CYBLE_EVT_GATT_CONNECT_IND:
			/* This flag is used in application to check connection status */
			Device_Connected = true;
			break;
            
	    case CYBLE_EVT_GATT_DISCONNECT_IND:
			/* This event is generated at GATT disconnection */
			Device_Connected = false;
            
        default:
    	    break;
    }
}

/*******************************************************************************
* Function Name: BAS_Event_Handler
********************************************************************************
*
* Summary:
*  This is an event callback function to receive events from the BLE Component
*   related to the Battery Alert Service (BAS). 
*
* Parameters:  
*  uint8 event:       Event from the CYBLE component
*  void* eventParams: A structure instance for corresponding event type. The 
*                     list of event structure is described in the component 
*                     datasheet.
*
* Return: 
*  None
*
*******************************************************************************/
void BAS_Event_Handler(uint32 event, void* eventParam)
{
    uint8 locServiceIndex;
    
    locServiceIndex = ((CYBLE_BAS_CHAR_VALUE_T *)eventParam)->serviceIndex;
    
    switch(event)
    {
        case CYBLE_EVT_BASS_NOTIFICATION_ENABLED:
            /* Battery Notification Enabled */
            if(CYBLE_BATTERY_SERVICE_INDEX == locServiceIndex)
            {
                Batt_Notification = ENABLED;
            }
            break;
                
        case CYBLE_EVT_BASS_NOTIFICATION_DISABLED:
            /* Battery Notification Disabled */
            if(CYBLE_BATTERY_SERVICE_INDEX == locServiceIndex)
            {
                Batt_Notification = DISABLED;
            }
            break;
        case CYBLE_EVT_BASC_NOTIFICATION:
            break;
        case CYBLE_EVT_BASC_READ_CHAR_RESPONSE:
            break;
        case CYBLE_EVT_BASC_READ_DESCR_RESPONSE:
            break;
        case CYBLE_EVT_BASC_WRITE_DESCR_RESPONSE:
            break;
		default:
			break;
    }
}

/*****************************************************************************
* Function Name: Send_BAS_Over_BLE
******************************************************************************
* Summary:
* Handles loading Battery Alert Service (BAS) data into BLE output packet
*
* Parameters:
* None
*
* Return:
* None
*
* Side Effects:
* None
*
*****************************************************************************/
void Send_BAS_Over_BLE(void)
{
    CYBLE_API_RESULT_T apiResult;
    
    if((BattResult.Data_Ready == true) && Batt_Notification)
    {
        /* Update Battery Level characteristic value */
        apiResult = CyBle_BassSendNotification(cyBle_connHandle, CYBLE_BATTERY_SERVICE_INDEX, CYBLE_BAS_BATTERY_LEVEL, 
                        sizeof(BattResult.Batt_Level), &BattResult.Batt_Level);
        if(apiResult != CYBLE_ERROR_OK)
        {
            /* BAS Error */
            Log_Error(BLE_PROCESS_ID, BLE_ERROR_BAS_ERROR);
        }
        else
        {
            /* BAS Success */
        }
        BattResult.Data_Ready = false;
    }
}

/*****************************************************************************
* Function Name: Send_Touch_Over_BLE
******************************************************************************
* Summary:
* Handles loading Battery Alert Service (BAS) data into BLE output packet
*
* Parameters:
* None
*
* Return:
* None
*
* Side Effects:
* None
*
*****************************************************************************/
void Send_Touch_Over_BLE(void)
{
    uint8 Touch_Packet[TOUCH_CHAR_DATA_LEN] = {0u};
    
    // TODO add debug signals to this function
    if((TouchResult.Data_Ready == true) && Touch_Notification)
    {
        /* send touch data to host client */
        Set32ByPtr(Touch_Packet, (uint8)TouchResult.CurrentCentroid);
        
        notificationHandle.attrHandle = CYBLE_TOUCH_SLIDER_CURRENT_CENTROID_CHAR_HANDLE;
        notificationHandle.value.val = Touch_Packet;
        notificationHandle.value.len = TOUCH_CHAR_DATA_LEN;
        
        CyBle_GattsNotification(cyBle_connHandle, &notificationHandle);
        
        /* Clear Flag indicating we have send the current data */
        TouchResult.Data_Ready = false;
    }    
}

/*****************************************************************************
* Function Name: Check_For_BLE_Data
******************************************************************************
* Summary:
* Responsible for checking all the possible new data flags that are set in the
*  BLE data handler ISR callback and updating the appropriate BLE database info.
*  Usually associted with Notification updates or Update Rate changes. 
*
* Parameters:
* None
*
* Return:
* None
*
* Side Effects:
* None
*
*****************************************************************************/
void Check_For_BLE_Data(void)
{
    uint8 Gatt_Temp[4] = {0,0,0,0};         /* Working Temp Variable */

    if(Update_Touch_Notification)
    {
        Set16ByPtr(Gatt_Temp, Touch_Notification);
        Update_Gatts_Attribute(CYBLE_TOUCH_SLIDER_CURRENT_CENTROID_CLIENT_CHARACTERISTIC_CONFIGURATION_DESC_HANDLE, Gatt_Temp, CCC_DATA_LEN);
        Update_Touch_Notification = false;
    }
}

/*****************************************************************************
* Function Name: Update_Gatts_Attribute
******************************************************************************
* Summary:
* Handles updating a particular Gatts Attribute in the BLE database, typically
*  after a new write has occurred. 
*
* Parameters:
* handle: Handle number for the attribute that needs to be updated
* data: Pointer to a uint8 array with the source data for the update
* length: Number of bytes to update
*
* Return:
* None
*
* Side Effects:
* None
*
*****************************************************************************/
void Update_Gatts_Attribute(CYBLE_GATT_DB_ATTR_HANDLE_T handle, uint8* data, uint8 length)
{
    /* Handle value to update the CCCD */
	CYBLE_GATT_HANDLE_VALUE_PAIR_T LocalHandle;
    
    /* Update database with latest data*/
	LocalHandle.attrHandle = handle;
	LocalHandle.value.val = data;
	LocalHandle.value.len = length;
    
    /* Report data to BLE component for sending data when read by Central device */
	CyBle_GattsWriteAttributeValue(&LocalHandle, 0, &cyBle_connHandle, CYBLE_GATT_DB_LOCALLY_INITIATED);
	
	/* Register the updated attribute write value to BLE component once before
	* updating the next attribute value */
	CyBle_ProcessEvents();
}

/* [] END OF FILE */
