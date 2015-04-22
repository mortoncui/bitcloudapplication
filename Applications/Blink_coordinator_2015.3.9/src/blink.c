/**************************************************************************//**
  \file blink.c

  \brief Blink application.

  \author mortoncui@qq.com using BITCLOUDY STACK API

  \internal
    History:2015/1/1
******************************************************************************/
#include <util/delay.h>	
#include <avr/io.h>
#include <appTimer.h>
#include <types.h>
#include <usart.h>
#include <zdo.h>
#include <blink.h>
#include <taskManager.h>
#include <leds.h>
#include <string.h>
#include <gpio.h>
#include <stdlib.h>
#define BULK_SIZE 500
#define Q_SIZE 3
#define APP_ASDU_SIZE 3
#define BUFFSIZE 10
#define CLEAR(x) memset(&(x),NULL,sizeof(x))
/**
* for start network                                                        
*/
BEGIN_PACK
typedef struct
{
	uint8_t header[APS_ASDU_OFFSET];
	uint8_t data[APP_ASDU_SIZE];
	uint8_t footer[APS_AFFIX_LENGTH-APS_ASDU_OFFSET];
}PACK AppMessageBuffer_t;
END_PACK
static AppMessageBuffer_t appMessageBuffer2;
volatile int human;
char human_s[5]={NULL};
static HAL_UsartDescriptor_t usartDescriptor;
static HAL_AppTimer_t appTimer;
static void Usart_Init(void);
static void saveData(uint8_t *message,uint16_t);
static ZDO_StartNetworkReq_t networkParams;
static void StartNetwork(void);
static void ZDO_StartNetworkConf(ZDO_StartNetworkConf_t *confirmInfo);
static void RstartNetwork(void);
static ZDO_StartNetworkReq_t networkParams;

/**
/* for endpoint     
/* simpleDescriptor: rec
/* simpleDescriptor2: push command                                               
*/
static void RegisterEndpoint(void);
static void APS_DataInd(APS_DataInd_t* indData);
static void APS_DataInd2(APS_DataInd_t* indData);
static SimpleDescriptor_t simpleDescriptor={1,1,1,1,0,0,NULL,0,NULL};
static SimpleDescriptor_t simpleDescriptor2={2,1,1,1,0,0,NULL,0,NULL};
static void usartReceieved(void);
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
/**
* Req data trans
*/
	static APS_DataReq_t dataReq2;
	static void Data2(unsigned char);
	static void APS_DataConf2(APS_DataConf_t *confInfo);
	static void Req_Table(ShortAddr_t shortAddr);
	static ZDO_ZdpReq_t zdpLeaveReq; //globally defined variable
	static volatile bool started=false;
/*****************************************************************************
                              Local variables section
******************************************************************************/
bool rxOnWhenIdle=0;
typedef struct
{
	bool sign;
	ShortAddr_t addr;
}ROUTER;
typedef enum
{
  APP_INITING_STATE,
  APP_STARTING_NETWORK_STATE,
  APP_IN_NETWORK_STATE,
  APP_LEAVING_NETWORK_STATE,
  APP_STOP_STATE,
  APP_CHILDJIONED_STATE,
  APP_COMMAND_STATE,
  READ_CHILD_STATE,
  WRITE_NET_PARA
} AppState_t;
typedef struct 
{
	int human;
	int temperature;
	int humid;
}Data_Receieved_t;
uint8_t rxBuffer[BULK_SIZE];
uint8_t txBuffer[BULK_SIZE];
uint8_t myBuffer[BULK_SIZE];
uint8_t getBuffer[BULK_SIZE];
uint8_t commandBuffer[BULK_SIZE];
Data_Receieved_t dataBuffers[Q_SIZE];
static AppState_t appState = APP_INITING_STATE;
ShortAddr_t  myshortAddr;
char buffer[BUFFSIZE];
//child_addr
static volatile NodeAddr_t childAddrTable[CS_MAX_CHILDREN_AMOUNT - CS_MAX_CHILDREN_ROUTER_AMOUNT];
static volatile ZDO_GetChildrenAddr_t children =
{
    .childrenCount = CS_MAX_CHILDREN_AMOUNT - CS_MAX_CHILDREN_ROUTER_AMOUNT,
    .childrenTable = childAddrTable,
};
void Timer_Handler(void)
{
	BSP_ToggleLed(LED_SECOND);
	//appState=READ_CHILD_STATE;
	//SYS_PostTask(APL_TASK_ID);
	//CLEAR(myBuffer);
	//CLEAR(commandBuffer);
}
static ZDO_Neib_t neighborTable[CS_NEIB_TABLE_SIZE]; 
static ZDO_ZdpReq_t zdpRequest;
void ZDO_ZdpResp(ZDO_ZdpResp_t* zdpResp);
typedef struct
{	uint8_t failures;
	bool multicast;
	uint32_t  polling;
	NwkLinkCost_t routeCost;   
}PARA;
static PARA global_para;
static PARA now_para;
static ROUTER router[2];
ROUTER *p_router=router;
/******************************************************************************
                              Implementations section
******************************************************************************/

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
		//CS_WriteParameter(CS_RX_ON_WHEN_IDLE_ID,&rxOnWhenIdle);
		Usart_Init();
		BSP_OpenLeds();
		BSP_OffLed(LED_FIRST);
		BSP_OffLed(LED_SECOND);

		appState=APP_STARTING_NETWORK_STATE;
		//strcpy(txBuffer,"msg from coo:network is starting!");
	   // HAL_WriteUsart(&usartDescriptor,txBuffer,strlen(txBuffer)*sizeof(char));
		SYS_PostTask(APL_TASK_ID);
	break;
	
	case APP_STARTING_NETWORK_STATE:
		StartNetwork();
	break;
		
	case APP_CHILDJIONED_STATE:
		{
			if(!started)
			appState=APP_IN_NETWORK_STATE;
		}
	break;  
	case APP_IN_NETWORK_STATE:
	{

		APS_RegisterEndpointReq(&endpointParams);
		APS_RegisterEndpointReq(&endpointParams2);
		appTimer.interval=3;
		appTimer.mode=TIMER_REPEAT_MODE;
		appTimer.callback=Timer_Handler;
		HAL_StartAppTimer(&appTimer);
		case APP_COMMAND_STATE:
		appState=appState;
	}	
	break;	
	case READ_CHILD_STATE:
		/*ZDO_GetChildrenAddr(&children);
		int actualNumberOfChildren = children.childrenCount;
		sprintf(myBuffer,"%d",actualNumberOfChildren);
		HAL_WriteUsart(&usartDescriptor,myBuffer,strlen(myBuffer));
		CLEAR(myBuffer);
		sprintf(myBuffer,"%x",sprintf(myBuffer,"%d",children.childrenTable->shortAddr));
		HAL_WriteUsart(&usartDescriptor,myBuffer,strlen(myBuffer));*/
		CLEAR(neighborTable);
		CLEAR(myBuffer);
		//p_router=router;//add 1
		//CLEAR(router);// add 2
		ZDO_GetNeibTable(neighborTable);
		ZDO_Neib_t *count=NULL;
			sprintf(myBuffer,"%s","[");
			HAL_WriteUsart(&usartDescriptor,myBuffer,strlen(myBuffer));	
			CLEAR(myBuffer);
		for(count=neighborTable;count-neighborTable<CS_NEIB_TABLE_SIZE;count++)
		{	
			char devcieType[10];
			CLEAR(devcieType);

			switch(count->deviceType)
			{
				case DEVICE_TYPE_COORDINATOR :
				sprintf(devcieType,"%s\n","->Coordinator  ");
				break;
				case DEVICE_TYPE_ROUTER :
				sprintf(devcieType,"%s\n","->Routor  ");
				{
					p_router->addr=count->shortAddr;
					if ((p_router-router)==1)
					{	
						p_router=router;						
					}else//ADD3
					{
					p_router++;
					}					
				}					
				//Req_Table(count->shortAddr);
				//_delay_ms(8000);
				break;		
				case DEVICE_TYPE_END_DEVICE :
				sprintf(devcieType,"%s\n","->End_Device  ");
				break;		
			}				
			sprintf(myBuffer,"%x",count->shortAddr);
		if(NULL==count->deviceType)
			{
					break;
			}				
			strcat(myBuffer,devcieType);
			HAL_WriteUsart(&usartDescriptor,myBuffer,strlen(myBuffer));
			CLEAR(myBuffer);
		}
		sprintf(myBuffer,"%s","]");
		HAL_WriteUsart(&usartDescriptor,myBuffer,strlen(myBuffer));
		CLEAR(myBuffer);	
		CLEAR(now_para);
		uint8_t paraBuffer[150];
		CS_ReadParameter(CS_INDIRECT_POLL_RATE_ID,&(now_para.polling)); 
		sprintf((char*)paraBuffer,"\r\nCS_INDIRECT_POLL_RATE_ID:%d\r\n",now_para.polling);
		HAL_WriteUsart(&usartDescriptor,paraBuffer,strlen(paraBuffer));		
		CS_ReadParameter(CS_MAX_NEIGHBOR_ROUTE_COST_ID,&(now_para.routeCost));
		sprintf((char*)paraBuffer,"\r\nCS_MAX_NEIGHBOR_ROUTE_COST:%d\r\n",now_para.routeCost);
		HAL_WriteUsart(&usartDescriptor,paraBuffer,strlen(paraBuffer));	
		CS_ReadParameter( CS_NWK_END_DEVICE_MAX_FAILURES_ID,&(now_para.failures));
		sprintf((char*)paraBuffer,"\r\nCS_NWK_END_DEVICE_MAX_FAILURES:%d\r\n",now_para.failures);
		HAL_WriteUsart(&usartDescriptor,paraBuffer,strlen(paraBuffer));	
		CS_ReadParameter(CS_NWK_USE_MULTICAST_ID,&(now_para.multicast));
		sprintf((char*)paraBuffer,"\r\nCS_NWK_USE_MULTICAST:%d\r\n",now_para.multicast);
		HAL_WriteUsart(&usartDescriptor,paraBuffer,strlen(paraBuffer));		
	break;
	case WRITE_NET_PARA:
	{
		uint16_t para;
		switch(strlen(commandBuffer))
		{
			case 2:
			para=1;
			break;
			case 3:
			para=commandBuffer[1]-'0';
			break;
			case 4:
			para=(commandBuffer[1]-'0')*10+(commandBuffer[2]-'0');
			break;
/*			case 6://for P1234Z
			{
				int a[4];
				int i;
				for(i=1;i<=4;i++)
				{
					if((commandBuffer[i]>='0')&&(commandBuffer[i]<='9'))
					{
						a[i-1]=commandBuffer[i]-'0';
					}else if((commandBuffer[i]>='a')&&(commandBuffer[i]<='f'))
					{
						a[i-1]=commandBuffer[i]-'a'+10;
					}						
				}
				para=a[0]*16*16*16+a[1]*16*16+a[2]*16+a[3];	
			CLEAR(commandBuffer);
			sprintf((char*)commandBuffer,"%x",para);
			HAL_WriteUsart(&usartDescriptor,commandBuffer,strlen(commandBuffer));	
			CLEAR(commandBuffer);											
			}				
			break;*/
			default:
			para=0;
			break;			
		}
	if (para)
	{
		switch(commandBuffer[0])
		{
			case 'G':
				
			break;
			case 'C':
				global_para.routeCost=para;//CS_WriteParameter(CS_NEIB_TABLE_SIZE, &para);
				CS_WriteParameter(CS_MAX_NEIGHBOR_ROUTE_COST_ID, (void*)&global_para.routeCost);
			break;
			case 'F':
				global_para.failures=para;//CS_WriteParameter(CS_NEIB_TABLE_SIZE, &para);
				CS_WriteParameter(CS_NWK_END_DEVICE_MAX_FAILURES_ID, (void*)&global_para.failures);
			break;
			case 'M':
				global_para.multicast=para;//CS_WriteParameter(CS_NEIB_TABLE_SIZE, &para);
				CS_WriteParameter(CS_NWK_USE_MULTICAST_ID, (void*)&global_para.multicast);
				break;
			case 'P':				
				appState=READ_CHILD_STATE;
				SYS_PostTask(APL_TASK_ID);															
			break;
			case 'R':
				RstartNetwork();
				/*appState=APP_STARTING_NETWORK_STATE;
				SYS_PostTask(APL_TASK_ID);*/
			break;	
			case 'Q':
			{
				zdpRequest.req.reqPayload.mgmtLqiReq.startIndex = 0;
				if (para==1)
				{
					Req_Table(router[0].addr);
				}
				if (para==2)
				{
					Req_Table(router[1].addr);
				}
			}				
			default:
			break;
		}		
	}
	CLEAR(commandBuffer);
	}
	
	break;	
	default:
	break;
	}	
}
/*******************************************************************************
  Description: Data trans.

  Parameters None

  Returns: None.
*******************************************************************************/
static void Data2(unsigned char address)
{
	//Assigning payload memory for the APS data request
	dataReq2.asdu = appMessageBuffer2.data;
	dataReq2.asduLength = sizeof(appMessageBuffer2.data);
	dataReq2.profileId = simpleDescriptor2.AppProfileId;
	dataReq2.dstAddrMode = APS_EXT_ADDRESS;
	//dataReq2.dstAddress.extAddress =0x2LL;
	dataReq2.dstAddress.extAddress =address;
	//dst Endpoint
	dataReq2.dstEndpoint = simpleDescriptor2.endpoint;                
	dataReq2.clusterId = CPU_TO_LE16(1);
	//src Endpoint
	dataReq2.srcEndpoint =CPU_TO_LE16(2);
	dataReq2.txOptions.acknowledgedTransmission = 1;
	dataReq2.txOptions.fragmentationPermitted=0;
	dataReq2.txOptions.indicateBroadcasts=1;
	dataReq2.radius = 1;
	dataReq2.APS_DataConf = APS_DataConf2;
	// Data transmission request
	APS_DataReq(&dataReq2);
}	
/*******************************************************************************
  Description: Data received indication.

  Parameters[in] indData

  Returns: None. for modify__different type£¡£¡£¡
*******************************************************************************/
void APS_DataInd(APS_DataInd_t* indData)
{
	// BSP_ToggleLed(LED_SECOND);
	//Assuming that the payload contains a structure of a particular type,
	 //convert asdu to that type
	 uint8_t *appMessage = (uint8_t  *) indData->asdu;
	 //Suppose the application processes not types of messages
	 //Save data to a buffer for further use
	 saveData(appMessage, indData->asduLength);
}	
/*******************************************************************************
  Description: Data received indication.

  Parameters None.

  Returns: None. for modify__different type£¡£¡£¡
*******************************************************************************/
void APS_DataInd2(APS_DataInd_t* indData)
{
	indData=indData;
}	
/*******************************************************************************
  Description: Data transform indication.

  Parameters[in] indData

  Returns: None. 
*******************************************************************************/
void APS_DataConf2(APS_DataConf_t *confInfo)
{
	BSP_ToggleLed(LED_FIRST);
}
/**************************************************************************//**
  \brief Start network

  \param None

  \return None.
******************************************************************************/
static void StartNetwork()
{
	bool predePANID=true;
	uint16_t nwkPANID = CPU_TO_LE16(0x1111);
	CS_WriteParameter(CS_NWK_PREDEFINED_PANID_ID,&predePANID);
	CS_WriteParameter(CS_NWK_PANID_ID, &nwkPANID);
	networkParams.ZDO_StartNetworkConf=ZDO_StartNetworkConf;//avoid lose child after reset
	ZDO_StartNetworkReq(&networkParams);
}	
static void ZDO_StartNetworkConf(ZDO_StartNetworkConf_t *confirmInfo)
{
	if (ZDO_SUCCESS_STATUS==confirmInfo->status)
	{
		//
	   // strcpy(txBuffer,"msg from coo:network is started!");
	  // HAL_WriteUsart(&usartDescriptor,txBuffer,strlen(txBuffer)*sizeof(char));
		BSP_OnLed(LED_FIRST);
		appState=APP_IN_NETWORK_STATE;
		SYS_PostTask(APL_TASK_ID);
		
	}else{
  
		//
	   // strcpy(txBuffer,"msg from coo:network error started!");
	   // HAL_WriteUsart(&usartDescriptor,txBuffer,strlen(txBuffer)*sizeof(char));
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
 usartDescriptor.rxBuffer = rxBuffer;
 usartDescriptor.rxBufferLength = sizeof(rxBuffer);
 usartDescriptor.txBuffer =txBuffer;
 usartDescriptor.txBufferLength = sizeof(txBuffer);
 usartDescriptor.rxCallback = usartReceieved;
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
	   BSP_OnLed(LED_FIRST);
  //nwkParams=nwkParams_global;
  switch (nwkParams->status)
  {
      case ZDO_NETWORK_STARTED_STATUS:
      break;
	  case ZDO_CHILD_JOINED_STATUS:
		  appState=APP_CHILDJIONED_STATE;
		  started=true;
		  sprintf(myBuffer,"%x",nwkParams->childInfo.shortAddr);
		  HAL_WriteUsart(&usartDescriptor,myBuffer,strlen(myBuffer));
		  CLEAR(myBuffer);
		  BSP_ToggleLed(LED_SECOND);
		  SYS_PostTask(APL_TASK_ID);
	  break;
	  case ZDO_CHILD_REMOVED_STATUS:
		  BSP_ToggleLed(LED_SECOND);
		  sprintf(myBuffer,"%x",nwkParams->childAddr.shortAddr);	  
		  HAL_WriteUsart(&usartDescriptor,myBuffer,strlen(myBuffer));
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
		 // strcpy(mesg,"noting to do!!!!\n");
		//  HAL_WriteUsart(&usartDescriptor,mesg,strlen(mesg));
		 // BSP_OffLed(LED_FIRST);
      break;

}

}
/*******************************************************************************
  Description: store data into quene.

  Parameters: char*.

  Returns: nothing.
*******************************************************************************/
void saveData(uint8_t* message,uint16_t lenght)
{	
	//uint8_t s[lenght];
	//memcpy(s,message,lenght);
	//while(lenght--)
	//{	
	//	sprintf(rxBuffer,"%d",s[0]);
	//	HAL_WriteUsart(&usartDescriptor,rxBuffer,strlen(rxBuffer)*sizeof(char));
	//}		
	
	human=*message;
	//sprintf(human_s,"%d",human);
	//strcpy(txBuffer,"human:");
	//strcat(txBuffer,human_s);
	//HAL_WriteUsart(&usartDescriptor,txBuffer,strlen(txBuffer)*sizeof(char));
}	
/*******************************************************************************
  Description: receieve data from usart

  Parameters: nothing.

  Returns: nothing.
*******************************************************************************/
void usartReceieved(void)
{
	HAL_ReadUsart(&usartDescriptor,buffer,sizeof(buffer));
	strcat(getBuffer,buffer);
	if (buffer[0]=='Z' || buffer[1]=='Z' || buffer[2]=='Z' || buffer[3]=='Z' || buffer[4]=='Z'|| buffer[5]=='Z'|| buffer[6]=='Z')
	{	
		CLEAR(commandBuffer);
		strcpy(commandBuffer,getBuffer);
		HAL_WriteUsart(&usartDescriptor,getBuffer,strlen(getBuffer)*sizeof(char));
		CLEAR(getBuffer);
		CLEAR(buffer);
		appState=WRITE_NET_PARA;
		SYS_PostTask(APL_TASK_ID);
	}		
}	
/*******************************************************************************
  Description: just a stub.

  Parameters: none.

  Returns: nothing.
*******************************************************************************/
void Req_Table(ShortAddr_t shortAddr)
{
	zdpRequest.reqCluster = MGMT_LQI_CLID; //Request type
	zdpRequest.dstAddrMode = SHORT_ADDR_MODE; //Addressing mode
	zdpRequest.dstNwkAddr = shortAddr; //Address of the destination
	//zdpRequest.req.reqPayload.mgmtLqiReq.startIndex = 0;
	zdpRequest.ZDO_ZdpResp = ZDO_ZdpResp; //Confirm callback
	ZDO_ZdpReq(&zdpRequest);
}	
/*******************************************************************************
  Description: just a stub.

  Parameters: none.

  Returns: nothing.
*******************************************************************************/
void ZDO_ZdpResp(ZDO_ZdpResp_t* zdpResp)
	{
	  uint8_t uartMsg[400];
	  uint8_t reqStatus = zdpResp->respPayload.status;
	  if(reqStatus == ZDO_SUCCESS_STATUS)
	 {
	  ZDO_MgmtLqiResp_t* lqiResp = &zdpResp->respPayload.mgmtLqiResp;
	  sprintf((char*)uartMsg, "----Neighbors received: %u----\r\n",
	  lqiResp->neighborTableListCount);
	 HAL_WriteUsart(&usartDescriptor,uartMsg,strlen(uartMsg)*sizeof(char));
	 NeighborTableList_t *table;
	 for(table=lqiResp->neighborTableList;
	 (table-lqiResp->neighborTableList)<lqiResp->neighborTableListCount;
	 table++)
	 {
		  sprintf((char*)uartMsg, "Device type: %u\r\nShort addr:%x\r\n",
		 table->deviceType,
		 table->networkAddr);
		 HAL_WriteUsart(&usartDescriptor,uartMsg,strlen(uartMsg)*sizeof(char)); 
	 }	 
	     sprintf((char*)uartMsg,"\r\n");
		 HAL_WriteUsart(&usartDescriptor,uartMsg,strlen(uartMsg)*sizeof(char)); 
		 if (zdpRequest.req.reqPayload.mgmtLqiReq.startIndex==0)
		 {
			 zdpRequest.req.reqPayload.mgmtLqiReq.startIndex=zdpRequest.req.reqPayload.mgmtLqiReq.startIndex+3;
			 ZDO_ZdpReq(&zdpRequest);			 
		 }
	 }
	 }	 
/*******************************************************************************
  Description: just a stub.

  Parameters: none.

  Returns: nothing.
*******************************************************************************/
void RstartNetwork(void)
{
	zdpLeaveReq.reqCluster = MGMT_LEAVE_CLID;
	zdpLeaveReq.dstAddrMode = SHORT_ADDR_MODE;
	zdpLeaveReq.dstNwkAddr = 0; // for own node address shall be 0
	zdpLeaveReq.ZDO_ZdpResp =NULL; // callback
	
		//for own node address shall be 0
	zdpLeaveReq.req.reqPayload. mgmtLeaveReq.deviceAddr = 0;
	//specify whether to force children leave or not
	zdpLeaveReq.req.reqPayload. mgmtLeaveReq.removeChildren = 1;
	//specify whether to perform rejoin procedure after network leave
	zdpLeaveReq.req.reqPayload. mgmtLeaveReq.rejoin = 1;
	ZDO_ZdpReq(&zdpLeaveReq); // request network leave
}
/*******************************************************************************
  Description: just a stub.

  Parameters: none.

  Returns: nothing.
*******************************************************************************/	 
/*******************************************************************************
  Description: just a stub.

  Parameters: none.

  Returns: nothing.
*******************************************************************************/
void ZDO_WakeUpInd(void)
{
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
