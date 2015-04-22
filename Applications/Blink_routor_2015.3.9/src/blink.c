/**************************************************************************//**
  \file blink.c

  \brief Blink application.

  \author mortoncui@qq.com using BITCLOUDY STACK API

  \internal
    History:2015/1/8
******************************************************************************/


#include <appTimer.h>
#include <types.h>
#include <usart.h>
#include <zdo.h>
#include <blink.h>
#include <taskManager.h>
#include <util/delay.h>	
#include <leds.h>
#include <string.h>
#include <gpio.h>
#define BULK_SIZE 500
#define Q_SIZE 3
#define BLINK_TIMES 6
#define APP_ASDU_SIZE 3
/**
* config data frame
*/
BEGIN_PACK
typedef struct
{
	uint8_t header[APS_ASDU_OFFSET];
	uint8_t data[APP_ASDU_SIZE];
	uint8_t footer[APS_AFFIX_LENGTH-APS_ASDU_OFFSET];
}PACK AppMessageBuffer_t;
END_PACK
static AppMessageBuffer_t appMessageBuffer;
/**
* Req data trans
*/
static APS_DataReq_t dataReq;
static void Data(void);
static void APS_DataConf(APS_DataConf_t *confInfo);
/**
* for start network                                                        
*/
static HAL_UsartDescriptor_t usartDescriptor;
static HAL_AppTimer_t appTimer;
static HAL_AppTimer_t appTimer2;
static void ZDO_WakeUpConf(ZDO_WakeUpConf_t *conf);
static void Usart_Init(void);
static void BLINKY(void);
static void saveData(int*,uint16_t);
static ZDO_StartNetworkReq_t networkParams;
static void StartNetwork(void);
static void ZDO_StartNetworkConf(ZDO_StartNetworkConf_t *confirmInfo);
static ZDO_StartNetworkReq_t networkParams;
/**
/* for endpoint                                                    
*/
static void RegisterEndpoint(void);
static void Timer_Handler(void);
static void Timer_Handler2(void);
static void APS_DataInd(APS_DataInd_t* indData);
static void APS_DataInd2(APS_DataInd_t* indData);
static SimpleDescriptor_t simpleDescriptor={1,1,1,1,0,0,NULL,0,NULL};
static SimpleDescriptor_t simpleDescriptor2={2,1,1,1,0,0,NULL,0,NULL};
static APS_RegisterEndpointReq_t endpointParams=
{
	.simpleDescriptor=&simpleDescriptor,
	.APS_DataInd=APS_DataInd,
};
static APS_RegisterEndpointReq_t endpointParams2=
{
	.simpleDescriptor=&simpleDescriptor2,
	.APS_DataInd=APS_DataInd2,
};
int blink;
/*****************************************************************************
                              Local variables section
******************************************************************************/
bool rxOnWhenIdle=0;
typedef enum
{
  APP_INITING_STATE,
  APP_STARTING_NETWORK_STATE,
  APP_IN_NETWORK_STATE,
  APP_LEAVING_NETWORK_STATE,
  APP_STOP_STATE,
  APP_CHILDJIONED_STATE,
  APP_CHILDREJIONED_STATE
} AppState_t;
typedef struct 
{
	int human;
	int temperature;
	int humid;
}Data_Receieved_t;
uint8_t rxBuffer[BULK_SIZE];
Data_Receieved_t dataBuffers[Q_SIZE];
char human_s[5]={NULL};
static AppState_t appState = APP_INITING_STATE;
volatile int sensor_state=0;
int restart=0;
/******************************************************************************
                              Implementations section
******************************************************************************/

/*******************************************************************************
  Description: timer task handler.

  Parameters: none.

  Returns: nothing.
*******************************************************************************/
void check_state(void)
{
	appTimer2.interval=5000;
	appTimer2.mode=TIMER_REPEAT_MODE;
	appTimer2.callback=Timer_Handler2;
	HAL_StartAppTimer(&appTimer2);
}	
void BLINKY(void)
{
	appTimer.interval=2000;
	appTimer.mode=TIMER_REPEAT_MODE;
	appTimer.callback=Timer_Handler;
	HAL_StartAppTimer(&appTimer);
}	

void Timer_Handler(void)
{
	if(blink++<BLINK_TIMES)
	{
		Data();
		BSP_ToggleLed(LED_SECOND);
	}else
	{
		HAL_StopAppTimer(&appTimer);
	}			
}
void Timer_Handler2(void)
{
	if(ZDO_OUT_NETWORK_STATUS==ZDO_GetNwkStatus())
	{
		ZDO_WakeUpReq_t req; 
		req.ZDO_WakeUpConf=ZDO_WakeUpConf;
		ZDO_WakeUpReq(&req);
	}		
}	
/*******************************************************************************
  Description: application task handler.

  Parameters: none.

  Returns: nothing.
*******************************************************************************/
void APL_TaskHandler(void)
{
	switch (appState)
	{
	case APP_INITING_STATE:	
		Usart_Init();
		BSP_OpenLeds();
		BSP_OffLed(LED_FIRST);
		BSP_OffLed(LED_SECOND);

		appState=APP_STARTING_NETWORK_STATE;
		strcpy(rxBuffer,"msg from coo:network is starting!");
	    HAL_WriteUsart(&usartDescriptor,rxBuffer,strlen(rxBuffer)*sizeof(char));
		SYS_PostTask(APL_TASK_ID);
	break;
	
	case APP_STARTING_NETWORK_STATE:
		StartNetwork();
	break;
		
	case APP_CHILDJIONED_STATE:
		BSP_OnLed(LED_FIRST);
		appState=APP_IN_NETWORK_STATE;
		appTimer.interval=800;
		appTimer.mode=TIMER_REPEAT_MODE;
		appTimer.callback=Timer_Handler;
		HAL_StartAppTimer(&appTimer);
		HAL_StopAppTimer(&appTimer2);
	break;  
	case APP_IN_NETWORK_STATE:
		APS_RegisterEndpointReq(&endpointParams);
		APS_RegisterEndpointReq(&endpointParams2);
	break;
	case APP_CHILDREJIONED_STATE:
		_delay_ms(2000);
		BSP_ToggleLed(LED_SECOND);
		check_state();
		appState=APP_STARTING_NETWORK_STATE;
		SYS_PostTask(APL_TASK_ID);
		
	break;
	default:
	break;
	}	
}
/*******************************************************************************
  Description: Data received indication.

  Parameters[in] indData

  Returns: None. for modify__different type£¡£¡£¡
*******************************************************************************/
void APS_DataInd(APS_DataInd_t* indData)
{
	indData=indData;
/*
	 BSP_OnLed(LED_FIRST);
	 int *appMessage = (int  *) indData->asdu;
	 saveData(appMessage, indData->asduLength - 1);
	 */
}	
void APS_DataInd2(APS_DataInd_t* indData)
{
	 BSP_ToggleLed(LED_FIRST);
	// printf("receieved!!!!");
	//Assuming that the payload contains a structure of a particular type,
	 //convert asdu to that type
	 uint8_t *appMessage = (uint8_t  *) indData->asdu;
	 //Suppose the application processes not types of messages
	 //Save data to a buffer for further use
	 saveData(appMessage, indData->asduLength);
}	
static void Data()
{
	//Assigning payload memory for the APS data request
	dataReq.asdu = appMessageBuffer.data;
	dataReq.asduLength = sizeof(appMessageBuffer.data);
	dataReq.profileId = 0x0001;
	dataReq.dstAddrMode = APS_SHORT_ADDRESS;
	dataReq.dstAddress.shortAddress = CPU_TO_LE16(0);
	dataReq.dstEndpoint = 1;
	dataReq.clusterId = CPU_TO_LE16(1);
	dataReq.srcEndpoint = 0x01;
	dataReq.txOptions.acknowledgedTransmission = 1;
	dataReq.radius = 0x0;
	dataReq.APS_DataConf = APS_DataConf;
	// Data transmission request
	APS_DataReq(&dataReq);
}	
/**************************************************************************//**
  \brief Start network

  \param None

  \return None.
******************************************************************************/
static void StartNetwork()
{
	bool rxOnWhenIdle = false;
	CS_WriteParameter(CS_RX_ON_WHEN_IDLE_ID,&rxOnWhenIdle);//make enddevice reveieve get & poll message
	networkParams.ZDO_StartNetworkConf=ZDO_StartNetworkConf;
	ZDO_StartNetworkReq(&networkParams);
}	
static void ZDO_StartNetworkConf(ZDO_StartNetworkConf_t *confirmInfo)
{
	if (ZDO_SUCCESS_STATUS==confirmInfo->status)
	{
		//
		restart=0;
		BSP_OnLed(LED_FIRST);
	    strcpy(rxBuffer,"msg from coo:network is started!");
	    HAL_WriteUsart(&usartDescriptor,rxBuffer,strlen(rxBuffer)*sizeof(char));
		BSP_OnLed(LED_FIRST);
		appState=APP_IN_NETWORK_STATE;
		APS_RegisterEndpointReq(&endpointParams);
		APS_RegisterEndpointReq(&endpointParams2);
		SYS_PostTask(APL_TASK_ID);
	}else{	
	restart=1;
	appState=APP_CHILDREJIONED_STATE;
	SYS_PostTask(APL_TASK_ID);
	}
	
}
	
void Usart_Init(void)
{
 usartDescriptor.tty = USART_CHANNEL_1;
 usartDescriptor.mode = USART_MODE_ASYNC; //Use asynchronous mode (UART)
 usartDescriptor.flowControl = USART_FLOW_CONTROL_NONE;
 usartDescriptor.baudrate = USART_BAUDRATE_38400;
 usartDescriptor.dataLength = USART_DATA8;
 usartDescriptor.parity = USART_PARITY_NONE;
 usartDescriptor.stopbits = USART_STOPBIT_1;
 usartDescriptor.rxBuffer = NULL;
 usartDescriptor.rxBufferLength = 0;
 usartDescriptor.txBuffer =rxBuffer;
 usartDescriptor.txBufferLength = sizeof(rxBuffer);
 usartDescriptor.rxCallback = NULL;
 usartDescriptor.txCallback = NULL;
 HAL_OpenUsart(&usartDescriptor);
}

/**************************************************************************//**
  \brief Indicates network parameters update.

  \param[in]  nwkParams - information about network updates.

  \return None.
******************************************************************************/
void ZDO_MgmtNwkUpdateNotf(ZDO_MgmtNwkUpdateNotf_t *nwkParams)
{
  char mesg[20]={0};
  switch (nwkParams->status)
  {
      case ZDO_NETWORK_STARTED_STATUS:
		  strcpy(mesg,"msg from coo:network is started!\n");
		  HAL_WriteUsart(&usartDescriptor,mesg,strlen(mesg));
		 // BSP_OffLed(LED_FIRST);
      break;
	  case ZDO_CHILD_JOINED_STATUS:
		  appState=APP_CHILDJIONED_STATE;
		  SYS_PostTask(APL_TASK_ID);
	  break;
	  case ZDO_NETWORK_LOST_STATUS :
		   BSP_OffLed(LED_FIRST);
		   appState=APP_CHILDREJIONED_STATE;
		   SYS_PostTask(APL_TASK_ID);
	  break;
	case ZDO_NO_NETWORKS_STATUS:
			appState=APP_CHILDREJIONED_STATE;
		   SYS_PostTask(APL_TASK_ID);
		   break;
    case ZDO_NO_KEY_PAIR_DESCRIPTOR_STATUS:
#ifdef _LINK_SECURITY_
      //The event can occur in high security mode on the trust center only
      //Extract the extended address of the device attempting to join the network
      ExtAddr_t addr = nwkParams->childInfo.extAddr;
      uint8_t   linkKey[16] = ...; //Find out the link key value and store in this variable
      APS_SetLinkKey(&addr, linkKey); //Set the link key for the given extended address
#endif
      break;
    default:
		  strcpy(mesg,"noting to do!!!!\n");
		  HAL_WriteUsart(&usartDescriptor,mesg,strlen(mesg));
		 // BSP_OffLed(LED_FIRST);
      break;

}

}
/**************************************************************************//**
  \brief Indicates network parameters update.

  \param None

  \return None.
******************************************************************************/
void APS_DataConf(APS_DataConf_t *confInfo)
{
	BSP_ToggleLed(LED_SECOND);
}	
/*******************************************************************************
  Description: store data into quene.

  Parameters: char*.

  Returns: nothing.
*******************************************************************************/
void saveData(int* message,uint16_t lenght)
{	
	//reserved
	message=message;
	lenght=lenght;
}	
/*******************************************************************************
  Description: just a stub.

  Parameters: none.

  Returns: nothing.
*******************************************************************************/
void ZDO_WakeUpInd(void)
{
	if (restart)
	{
		appState=APP_CHILDREJIONED_STATE;
		SYS_PostTask(APL_TASK_ID);
	}
}

void ZDO_WakeUpConf(ZDO_WakeUpConf_t *conf)
{
	if (restart)
	{
		appState=APP_CHILDREJIONED_STATE;
		SYS_PostTask(APL_TASK_ID);
	}
}	
#ifdef _BINDING_
/***********************************************************************************
  Stub for ZDO Binding Indication

  Parameters:
    bindInd - indication

  Return:
    none

 ***********************************************************************************/
void ZDO_BindIndication(ZDO_BindInd_t *bindInd)
{
  (void)bindInd;
}

/***********************************************************************************
  Stub for ZDO Unbinding Indication
3
  Parameters:
    unbindInd - indication

  Return:
    none

 ***********************************************************************************/
void ZDO_UnbindIndication(ZDO_UnbindInd_t *unbindInd)
{
  (void)unbindInd;
}
#endif //_BINDING_

/**********************************************************************//**
  \brief Main - C program main start function

  \param none
  \return none
**************************************************************************/
int main(void)
{
  SYS_SysInit();

  for(;;)
  {
    SYS_RunTask();
  }
}

//eof blink.c
