/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdint.h"
#include "stdio.h"
#include <stdlib.h>
#include "rtc.h"
#include "usart.h"
#include <command.h>
#include "cmdline.h"
#include "fatfs.h"
#include <string.h>
#include "modbus_mqtt_bridge.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
FATFS fs;
FRESULT fre;
FIL fil;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
/* Shared variables ------------------*/

FATFS fs;
FIL fil;
FRESULT fresult,fre;


char SDbuffer[300];
static data1_t *ptr;
data1_t *dynamic;
uint8_t num_device;

extern osThreadId mbProtocolTask;
extern osThreadId mbAppTask;
extern osThreadId mbDownlinkTask; // HERE
extern osThreadId netTimeTask;
extern osThreadId netHTTPTask;
extern osThreadId netMQTTTask;
extern osThreadId netTcpEchoTask;

osThreadId resetHandlerTask;
osThreadId flashTask;

extern uint32_t gotCommandFlag;
extern uint8_t commandBuffer[200];
extern uint32_t commandBufferIndex;
extern uint8_t record[200];

osMessageQId xQueueControlHandle;
osMessageQId xQueueMessageHandle;
osMessageQId xQueueDownlinkHandle;
osMessageQId xQueueUplinkHandle;
osMessageQId xQueueResetHandle;
/* USER CODE END Variables */
osThreadId defaultTaskHandle;
osTimerId myTimer01Handle;
osSemaphoreId netMqttIpSemaphoreHandle;
xQueueMbMqtt_t xQueueMbMqtt;
/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
extern void ModbusRTUTask(void const * argument);
extern void ModbusTestTask(void const * argument);
extern void ModbusDownlinkTask(void const * argument); //HERE
extern void http_server_netconn_init();
extern void NetworkTimeTask(void const *arg);
extern void TcpDiscoverTask(void *arg);
extern void http_server_netconn_thread();
extern void netmqttTask(void const *arg);
void ResetHandlerTask(void *arg);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
void Callback01(void const * argument);

extern void MX_LWIP_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* GetTimerTaskMemory prototype (linked to static allocation support) */
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize );

/* Hook prototypes */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName);
void vApplicationMallocFailedHook(void);
void vApplicationDaemonTaskStartupHook(void);

/* USER CODE BEGIN 4 */
__weak void vApplicationStackOverflowHook(xTaskHandle xTask,
		signed char *pcTaskName) {
	printf("\r\n Stack overflow: %s\r\n",pcTaskName);
	/* Run time stack overflow checking is performed if
	 configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
	 called if a stack overflow is detected. */
}
/* USER CODE END 4 */

/* USER CODE BEGIN 5 */
__weak void vApplicationMallocFailedHook(void) {
	/* vApplicationMallocFailedHook() will only be called if
	 configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h. It is a hook
	 function that will get called if a call to pvPortMalloc() fails.
	 pvPortMalloc() is called internally by the kernel whenever a task, queue,
	 timer or semaphore is created. It is also called by various parts of the
	 demo application. If heap_1.c or heap_2.c are used, then the size of the
	 heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
	 FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
	 to query the size of free heap space that remains (although it does not
	 provide information on how the remaining heap might be fragmented). */
}
/* USER CODE END 5 */

/* USER CODE BEGIN DAEMON_TASK_STARTUP_HOOK */
void vApplicationDaemonTaskStartupHook(void) {
}
/* USER CODE END DAEMON_TASK_STARTUP_HOOK */

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
		StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize) {
	*ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
	*ppxIdleTaskStackBuffer = &xIdleStack[0];
	*pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
	/* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/* USER CODE BEGIN GET_TIMER_TASK_MEMORY */
static StaticTask_t xTimerTaskTCBBuffer;
static StackType_t xTimerStack[configTIMER_TASK_STACK_DEPTH];

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
		StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize) {
	*ppxTimerTaskTCBBuffer = &xTimerTaskTCBBuffer;
	*ppxTimerTaskStackBuffer = &xTimerStack[0];
	*pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
	/* place for user code */
}
/* USER CODE END GET_TIMER_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
	/* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* definition and creation of netMqttIpSemaphore */
  osSemaphoreDef(netMqttIpSemaphore);
  netMqttIpSemaphoreHandle = osSemaphoreCreate(osSemaphore(netMqttIpSemaphore), 1);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
	/* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* Create the timer(s) */
  /* definition and creation of myTimer01 */
  osTimerDef(myTimer01, Callback01);
  myTimer01Handle = osTimerCreate(osTimer(myTimer01), osTimerPeriodic, NULL);

  /* USER CODE BEGIN RTOS_TIMERS */
	/* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
	/* add queues, ... */

	osMessageQDef(xQueueControl, 1, xQueueControl_t);
	xQueueControlHandle = osMessageCreate(osMessageQ(xQueueControl), NULL);

	osMessageQDef(xQueueMessage, 1, uint8_t);
	xQueueMessageHandle = osMessageCreate(osMessageQ(xQueueMessage), NULL);

	osMessageQDef(xQueueDownlink, 1, xQueueMbMqtt_t);
	xQueueDownlinkHandle = osMessageCreate(osMessageQ(xQueueDownlink), NULL);

	osMessageQDef(xQueueUplink, 1, xQueueMbMqtt_t);
	xQueueUplinkHandle = osMessageCreate(osMessageQ(xQueueUplink), NULL);

	osMessageQDef(xQueueReset, 1, sizeof(uint32_t));
	xQueueResetHandle = osMessageCreate(osMessageQ(xQueueUplink), NULL);

  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 1500);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
	/* add threads, ... */

  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* init code for LWIP */
	if (LoadSdcard("config.txt") == 1){
		MX_LWIP_Init();
		USER_USART2_UART_Init();
		USER_USART3_UART_Init();
	}
  /* USER CODE BEGIN StartDefaultTask */
	/*Create Task Modules in this line*/

	/*Modbus protocol stack*/
	osThreadDef(mbProtocolTask, ModbusRTUTask, osPriorityNormal, 0, 4 * 128);
	mbProtocolTask = osThreadCreate(osThread(mbProtocolTask), NULL);
	printf("\r\n MemFree: %d", xPortGetFreeHeapSize());

	/*Modbus app*/
	osThreadDef(mbAppTask, ModbusTestTask, osPriorityNormal, 0, 2 * 128);
	mbAppTask = osThreadCreate(osThread(mbAppTask), NULL);
	printf("\r\n MemFree: %d", xPortGetFreeHeapSize());

	/*Modbus Downlink */
	osThreadDef(mbDownlinkTask, ModbusDownlinkTask,osPriorityRealtime, 0, 2 * 128);
	mbDownlinkTask= osThreadCreate(osThread(mbDownlinkTask), NULL);
	printf("\r\n MemFree: %d", xPortGetFreeHeapSize());

	/*Get time*/
	osThreadDef(netTimeTask, NetworkTimeTask, osPriorityNormal, 0, 4 * 128);
	netTimeTask = osThreadCreate(osThread(netTimeTask), NULL);
	printf("\r\n MemFree: %d", xPortGetFreeHeapSize());

	/*Tcp Server for CORE discovery*/
	osThreadDef(netTcpEchoTask, TcpDiscoverTask, osPriorityNormal, 0, 2 * 128);
	netTcpEchoTask = osThreadCreate(osThread(netTcpEchoTask), NULL);
	printf("\r\n MemFree: %d", xPortGetFreeHeapSize());

	/*MQTT serive*/
	osThreadDef(netMQTTTask, netmqttTask, osPriorityNormal, 0, 14 * 256);
	netMQTTTask = osThreadCreate(osThread(netMQTTTask), NULL);
	printf("\r\n MemFree: %d", xPortGetFreeHeapSize());

	/*HTTP service*/
	osThreadDef(netHTTPTask, http_server_netconn_thread, osPriorityNormal, 0,2 * 128);
	netHTTPTask = osThreadCreate(osThread(netHTTPTask), NULL);
	if (netHTTPTask == NULL) {
		printf("\r\n netHTTP task cannot be created!");
	}
	/*Reset Handler*/
	osThreadDef(resetHandlerTask, ResetHandlerTask, osPriorityRealtime, 0, 128);
	resetHandlerTask = osThreadCreate(osThread(resetHandlerTask), NULL);

	printf("\r\n\r\n MemFree: %d", xPortGetFreeHeapSize());

	/*Controller*/
	xQueueControl_t xQueueControl;
	#define PORT_DEFAULT_DELAY	1000
	uint8_t uiSysState = SYS_START;
	uint8_t uiSysUpdate = FALSE;
	BaseType_t sysError;
	int ret;
	char *Badcommand = "Bad Command\r\n>";
	/* Infinite loop */
	for (;;) {
		switch (uiSysState) {
		case SYS_START:
//			if (LoadSdcard("config.txt") == 0){
//				__disable_irq();
//				xFlashLoad();
//				__enable_irq();
//			}
//			MX_LWIP_Init();
//			USER_USART2_UART_Init();
//			USER_USART3_UART_Init();
			printf("\r\n Task Controller: Implementing...  \r\n");
			uiSysUpdate = TRUE;
			uiSysState++;
			break;
		case SYS_MB_PROTOCOL:
			printf("\r\n Starting mbProtocolTask module:  \r\n");
			sysError = xQueueReceive(xQueueControlHandle, &xQueueControl, portMAX_DELAY);
			if (sysError == pdPASS) {
				if ((xQueueControl.xState == TASK_RUNNING)&& (xQueueControl.xTask == mbProtocolTask)) {
					printf("\r\n Starting mbProtocolTask module: OK \r\n");
					uiSysUpdate = TRUE;
					uiSysState++;
				}
			} else {
				printf("\r\n Starting mbProtocolTask module: Responding Timeout \r\n");
			}
			break;
		case SYS_MB_APP:
			printf("\r\n Starting mbAppTask module:  \r\n");
			sysError = xQueueReceive(xQueueControlHandle, &xQueueControl, portMAX_DELAY);
			if (sysError == pdTRUE) {
				if ((xQueueControl.xState == TASK_RUNNING) && (xQueueControl.xTask == mbAppTask)) {
					printf("\r\n Starting mbAppTask module: OK \r\n");
					uiSysUpdate = TRUE;
					uiSysState++;

				}
			} else {
				printf("\r\n Starting Modbus Uplink module: Responding Timeout \r\n");
			}
			break;
		case SYS_MB_DOWNLINK:
			printf("\r\n Starting mbDownlinkTask module:  \r\n");
			sysError = xQueueReceive(xQueueControlHandle, &xQueueControl,portMAX_DELAY);
			if (sysError == pdTRUE) {
				if ((xQueueControl.xState == TASK_RUNNING) && (xQueueControl.xTask == mbDownlinkTask)) {
					printf("\r\n Starting mbDownlinkTask module: OK \r\n");
					uiSysUpdate = TRUE;
					uiSysState++;
				}
			} else {
				printf("\r\n Starting mbDownlinkTask module: Responding Timeout \r\n");
			}
			break;

		case SYS_NET_TIME:
			#define portDEFAULT_DELAY 1000
			printf("\r\n Starting netTimeTask");
			sysError = xQueueReceive(xQueueControlHandle, &xQueueControl,
			portDEFAULT_DELAY);
			if (sysError == pdTRUE) {
				if ((xQueueControl.xState == TASK_RUNNING) && (xQueueControl.xTask == netTimeTask)) {
					printf("\r\n Starting netTimeTask  module: OK \r\n");
					uiSysUpdate = TRUE;
					uiSysState++;
					/*Warning - Delete netTimeTask to save resource !!*/
					/*Update  - Resource is a ocean so we shouldn't think about it if we don't want to get any bugs*/
//					vTaskDelete(netTimeTask);
//					printf("\r\n Killing netTimeTask module: OK \r\n");
				}
			} else {
				printf("\r\n Starting netTimeTask module: Responding Timeout \r\n");
				uiSysUpdate = TRUE;
				uiSysState++;
			}
			break;
		case SYS_CORE_DISCOV:
			printf("\r\n Waiting for CORE discovery in %d ms:  \r\n",
			portDEFAULT_DELAY);
			sysError = xQueueReceive(xQueueControlHandle, &xQueueControl,
			portDEFAULT_DELAY);
			if (sysError == pdTRUE) {
				if ((xQueueControl.xState == TASK_RUNNING)
						&& (xQueueControl.xTask == netTcpEchoTask)) {
					printf("\r\n  Waiting for CORE discovery: OK \r\n");
					uiSysUpdate = TRUE;
					uiSysState++;
				} else if ((xQueueControl.xState == TASK_ERR_1)
						&& (xQueueControl.xTask == netTcpEchoTask)) {
					printf("\r\n  Waiting for CORE discovery: Timeout, use pre-configured MQTT server \r\n");
				}
			} else {
				printf("\r\n Waiting for CORE discovery: Timeout \r\n");
				uiSysUpdate = TRUE;
				uiSysState++;
			}
			break;
		case SYS_MQTT:
			printf("\r\n Starting netMQTTTask module:  \r\n");
			sysError = xQueueReceive(xQueueControlHandle, &xQueueControl,
			portDEFAULT_DELAY);
			if (sysError == pdTRUE) {
				if ((xQueueControl.xState == TASK_RUNNING)
						&& (xQueueControl.xTask == netMQTTTask)) {
					printf("\r\n Starting netMQTTTask module: OK \r\n");
					uiSysUpdate = TRUE;
					uiSysState++;
				}
			} else {
				printf(
						"\r\n Starting netMQTTTask module: Responding Timeout \r\n");
				uiSysUpdate = TRUE;
				uiSysState++;
			}
			break;
		case SYS_HTTP:
			sysError = xQueueReceive(xQueueControlHandle, &xQueueControl,
			portMAX_DELAY);
			if (sysError == pdTRUE) {
				if ((xQueueControl.xState == TASK_RUNNING)
						&& (xQueueControl.xTask == netHTTPTask)) {
					printf("\r\n Starting netHTTPTask module: OK \r\n");
					uiSysUpdate = TRUE;
					uiSysState++;

				}
			} else {
				printf(
						"\r\n Starting netHTTPTask module: Responding Timeout \r\n");
				uiSysUpdate = TRUE;
				uiSysState++;
			}
			break;
		case SYS_RECORD:
			printf("\r\n SYS_RECORD: Starting...  \r\n");
			xQueueMbMqtt_t xQueueMbMqtt;
			BaseType_t Err = pdFALSE;
			if (CheckRecord("record.txt") == 1){
				xQueueMbMqtt.gotflagLast = 1;
				//HAL_Delay(2000);
				Err = xQueueSend(xQueueUplinkHandle, &xQueueMbMqtt,portDEFAULT_DELAY);
				if (Err == pdPASS){
					xQueueMbMqtt.gotflagLast = 0;
					//printf("\r\n-----------------ALL DATA IN RECORD: SENT--------------------\r\n");
				}
			}
			uiSysUpdate = TRUE;
			uiSysState++;
			break;
		case SYS_DEVICE:
			printf("\r\n SYS_DEVICE: Implementing...  \r\n");
			MX_FATFS_Init();
			static uint8_t line;
			dynamic = NULL;
//			if (BSP_SD_Init() == MSD_OK)
//				{
					fresult = f_mount(&fs, "/", 1);
					fresult = f_open(&fil,"device.txt", FA_READ);
					for (line= 0; (f_eof(&fil) == 0); line++)
						{
							f_gets((char*)SDbuffer, sizeof(SDbuffer), &fil);
						}
					fresult = f_close(&fil);
					num_device = line;
					printf("\r\nNum of line: %d\r\n", line);
					dynamic = (data1_t*)pvPortMalloc(line * sizeof(data1_t));
					if (dynamic != NULL){
						fresult = f_mount(&fs, "/", 1);
						fresult = f_open(&fil,"device.txt", FA_READ);
						for (uint8_t i = 0; (f_eof(&fil) == 0); i++)
							{
								memset(SDbuffer,'\0',sizeof(SDbuffer));
								f_gets((char*)SDbuffer, sizeof(SDbuffer), &fil);
								ptr = parse_device((char*)SDbuffer, strlen(SDbuffer));
								addDevice((dynamic+i), ptr);
							}
						printf("\r\n");
						fresult = f_close(&fil);
						HAL_GPIO_WritePin(USART2_LED_GPIO_Port,USART2_LED_Pin, GPIO_PIN_SET);
						HAL_GPIO_WritePin(USART3_LED_GPIO_Port,USART3_LED_Pin , GPIO_PIN_SET);
						HAL_Delay(3000);
						HAL_GPIO_WritePin(USART2_LED_GPIO_Port,USART2_LED_Pin, GPIO_PIN_RESET);
						HAL_GPIO_WritePin(USART3_LED_GPIO_Port,USART3_LED_Pin , GPIO_PIN_RESET);
					}
					else if (dynamic == NULL){
						printf("\r\n-----------------------ALLOCATING MEMORY FAIL------------------------\r\n");
					}
					//vPortFree(dynamic);
//				}
				uiSysUpdate = TRUE;
				uiSysState++;
			break;
		case SYS_DEFAULT:
			if (gotCommandFlag == 1) {
				gotCommandFlag = 0;
				ret = CmdLineProcess(commandBuffer);
				if (ret == CMDLINE_BAD_CMD)
					HAL_UART_Transmit(&huart6, Badcommand, strlen(Badcommand),100);
			}
			//vTaskDelay(10);
			//osDelay(10);
			break;

		}
		if ((uiSysUpdate == TRUE) && (uiSysState != SYS_DEFAULT)) {
			xQueueSend(xQueueMessageHandle, &uiSysState, 0);
			uiSysUpdate = FALSE;
		}
//		osDelay(200);
	}
  /* USER CODE END StartDefaultTask */
}
/*----------------------------------------------------------------------------------------------*/
/* Callback01 function */
void Callback01(void const * argument)
{
  /* USER CODE BEGIN Callback01 */
	/*FreeRTOS Timer API for Modbus protocol stack*/
	prvvMasterTIMERExpiredISR();
  /* USER CODE END Callback01 */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/*Brief: Waiting until semaphore do someting
 * 	+ Flash save
 * 	+ Reset Machine
 *
 * */
void FlashTask(void *arg) {
	//xFlashSave();
	//vTaskDelay(100);
	//NVIC_SystemReset();
	while (1)
		;
}
void ResetHandlerTask(void *arg)
{
	BaseType_t err = pdFALSE;
	uint32_t handle;
	while (1)
	{
		err = xQueueReceive(xQueueResetHandle, &handle, portMAX_DELAY);
		if (err == pdTRUE) {
			if (handle == 1) {
				osThreadDef(flashSave, FlashTask, osPriorityRealtime,0, 2 * 128);
				flashTask = osThreadCreate(osThread(flashSave), NULL);
				//xFlashSave();
				//osDelay(10);
				printf("\r\n System reset \r\n");
				NVIC_SystemReset();
			}
		}
		vTaskDelay(100);
	}
}

/* USER CODE END Application */
