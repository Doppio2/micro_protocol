/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
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
#include "usart.h"
#include <string.h>
#include <stdio.h>
#include "ll_protocol.h"
#include "hardware.h"
#include "uart_connection.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

// Структура для хранения данных в очереди PMB Monitoring Task
typedef struct
{
	float voltage;
	float current;
} PMB_Data_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TELEMETRY_QUEUE_SET_LENGTH 2 // длина набора очередей в задачу сбора телеметрии (2 очереди)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
SemaphoreHandle_t xUART_TxCpltSemaphore;   // семафор синхронизации задачи отправки телеметрии
SemaphoreHandle_t xUART_RxCpltSemaphore;   // семафор синхронизации обработки пакета
SemaphoreHandle_t xCMD_TelemetrySemaphore; // семафор синхронизации задачи отправки телеметрии
SemaphoreHandle_t xCMD_FirmwareSemaphore;  // семафор синхронизации обновления прошивки
QueueHandle_t xDataRecievedQueue;		   // очередь принятых по UART данных (пакетов)
QueueHandle_t xThrustersControlQueue;	   // очередь данных для управления движителями
QueueHandle_t xTelemetryToSendQueue;	   // очередь данных для отправки телеметрии по UART (overwrite queue)
QueueHandle_t xTelemetryFromPMBTaskQueue;
QueueSetHandle_t xTelemetryQueueSet; // набор очередей для задачи сбора телеметрии
/* USER CODE END Variables */
osThreadId defaultTaskHandle;
osThreadId TelemetryCollecHandle;
osThreadId CMDTaskHandle;
osThreadId FirmUpdateTaskHandle;
osThreadId RxUARTTaskHandle;
osThreadId SendTelemetryTaHandle;
osThreadId ThrustersControlTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const *argument);
void StartTelemetryCollectTask(void const *argument);
void StartCommandProcessingTask(void const *argument);
void StartFirmwareUpdateTask(void const *argument);
void StartRxUARTTask(void const *argument);
void StartSendTelemetryTask(void const *argument);
void StartThrustersControlTask(void const *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize);

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize)
{
	*ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
	*ppxIdleTaskStackBuffer = &xIdleStack[0];
	*pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
	/* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
 * @brief  FreeRTOS initialization
 * @param  None
 * @retval None
 */
void MX_FREERTOS_Init(void)
{
	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* USER CODE BEGIN RTOS_MUTEX */
	/* add mutexes, ... */
	/* USER CODE END RTOS_MUTEX */

	/* USER CODE BEGIN RTOS_SEMAPHORES */
	/* add semaphores, ... */
	xUART_RxCpltSemaphore = xSemaphoreCreateBinary();
	xUART_TxCpltSemaphore = xSemaphoreCreateBinary();
	xCMD_TelemetrySemaphore = xSemaphoreCreateBinary();
	xCMD_FirmwareSemaphore = xSemaphoreCreateBinary();
	/* USER CODE END RTOS_SEMAPHORES */

	/* USER CODE BEGIN RTOS_TIMERS */
	/* start timers, add new ones, ... */
	/* USER CODE END RTOS_TIMERS */

	/* USER CODE BEGIN RTOS_QUEUES */
	/* add queues, ... */
	xDataRecievedQueue = xQueueCreate(5, sizeof(PacketReceived_t));
	xTelemetryToSendQueue = xQueueCreate(1, sizeof(Telemetry)); // эта очередь должна иметь размер 1 элемент (overwrite queue)
	xThrustersControlQueue = xQueueCreate(1, sizeof(ThrustersControl));
	xTelemetryFromPMBTaskQueue = xQueueCreate(5, sizeof(PMB_Data_t));
	xTelemetryQueueSet = xQueueCreateSet(TELEMETRY_QUEUE_SET_LENGTH);
	/* USER CODE END RTOS_QUEUES */

	/* Create the thread(s) */
	/* definition and creation of defaultTask */
	osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
	defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

	/* definition and creation of TelemetryCollec */
	osThreadDef(TelemetryCollec, StartTelemetryCollectTask, osPriorityNormal, 0, 128);
	TelemetryCollecHandle = osThreadCreate(osThread(TelemetryCollec), NULL);

	/* definition and creation of CMDTask */
	osThreadDef(CMDTask, StartCommandProcessingTask, osPriorityAboveNormal, 0, 128);
	CMDTaskHandle = osThreadCreate(osThread(CMDTask), NULL);

	/* definition and creation of FirmUpdateTask */
	osThreadDef(FirmUpdateTask, StartFirmwareUpdateTask, osPriorityHigh, 0, 128);
	FirmUpdateTaskHandle = osThreadCreate(osThread(FirmUpdateTask), NULL);

	/* definition and creation of RxUARTTask */
	osThreadDef(RxUARTTask, StartRxUARTTask, osPriorityAboveNormal, 0, 128);
	RxUARTTaskHandle = osThreadCreate(osThread(RxUARTTask), NULL);

	/* definition and creation of SendTelemetryTa */
	osThreadDef(SendTelemetryTa, StartSendTelemetryTask, osPriorityHigh, 0, 128);
	SendTelemetryTaHandle = osThreadCreate(osThread(SendTelemetryTa), NULL);

	/* definition and creation of ThrustersControlTask */
	osThreadDef(ThrustersControlTask, StartThrustersControlTask, osPriorityAboveNormal, 0, 128);
	ThrustersControlTaskHandle = osThreadCreate(osThread(ThrustersControlTask), NULL);

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
void StartDefaultTask(void const *argument)
{
	/* USER CODE BEGIN StartDefaultTask */
	/* Infinite loop */
	for (;;)
	{
		osDelay(1);
	}
	/* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartTelemetryCollectTask */
/**
 * @brief Function implementing the TelemetryCollec thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartTelemetryCollectTask */
void StartTelemetryCollectTask(void const *argument)
{
	/* USER CODE BEGIN StartTelemetryCollectTask */
	/* Infinite loop */
	for (;;)
	{
		osDelay(1);
	}
	/* USER CODE END StartTelemetryCollectTask */
}

/* USER CODE BEGIN Header_StartCommandProcessingTask */
/**
 * @brief Function implementing the CMDTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartCommandProcessingTask */
void StartCommandProcessingTask(void const *argument)
{
	/* USER CODE BEGIN StartCommandProcessingTask */
	static PacketReceived_t packet;
	static ThrustersControl thrusters_control;
	/* Infinite loop */
	for (;;)
	{
		/* Ожидаем пришедший пакет в очереди*/
		xQueueReceive(xDataRecievedQueue, &packet, portMAX_DELAY);

		/* Парсинг пакета */
		size_t payload_len = 0;
		uint8_t *payload_pt = packetParsing(packet.data, packet.length, &payload_len);
		size_t data_len = 0;
		uint8_t *data_pt = getDataPointFromPayload(payload_pt, payload_len, &data_len); // data from PAYLOAD
		pb_istream_t istream;
		if (data_pt != NULL)
			istream = pb_istream_from_buffer(data_pt, data_len);

		if (payload_pt != NULL)
		{
			/* Обработка PAYLOAD */
			CommandType_t cmd_type = getCommandType(payload_pt, payload_len);
			switch (cmd_type)
			{
			case CMD_UPDATE_FIRMWARE:
				/* Активируем задачу обновления прошивки */
				xSemaphoreGive(xCMD_FirmwareSemaphore);
				break;
			case CMD_GET_TELEMETRY:
				/* Активируем задачу отправки телеметрии */
				xSemaphoreGive(xCMD_TelemetrySemaphore);
				break;
			case CMD_THRUSTERS_CONTROL: //
				/* Декодируем сообщение и отправляем в очереди*/
				if (data_pt != NULL)
					if (pb_decode(&istream, ThrustersControl_fields, &thrusters_control) == true)
						xQueueOverwrite(xThrustersControlQueue, &thrusters_control);
#ifdef DEBUG_ENABLE
				printf("INFO: CMD_POWER_SWITCH\r\n");
#endif
				break;
			case CMD_HARDWARE_CONTROL:
				//
#ifdef DEBUG_ENABLE
				printf("INFO: CMD_NONE\r\n");
#endif
				break;
			case CMD_NONE:
#ifdef DEBUG_ENABLE
				printf("INFO: CMD_NONE\r\n");
#endif
				break;
			default:
				break;
			}
		}
	}
	/* USER CODE END StartCommandProcessingTask */
}

/* USER CODE BEGIN Header_StartFirmwareUpdateTask */
/**
 * @brief Function implementing the FirmUpdateTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartFirmwareUpdateTask */
void StartFirmwareUpdateTask(void const *argument)
{
	/* USER CODE BEGIN StartFirmwareUpdateTask */
	/* Infinite loop */
	for (;;)
	{
		xSemaphoreTake(xCMD_FirmwareSemaphore, portMAX_DELAY);
		printf("FIRMWARE UPDATE\r\n");
		JumpToBootloader();
		// osDelay(1);
	}
	/* USER CODE END StartFirmwareUpdateTask */
}

/* USER CODE BEGIN Header_StartRxUARTTask */
/**
 * @brief Function implementing the RxUARTTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartRxUARTTask */
void StartRxUARTTask(void const *argument)
{
	/* USER CODE BEGIN StartRxUARTTask */
	static PacketReceived_t packet;

	initUARTtoRx();
	/* Infinite loop */
	for (;;)
	{
		startReadUARTtoBuffer();

		/* Ожидаем конец пакета данных с таймаутом UART_RX_TIMEOUT_MS */
		if (xSemaphoreTake(xUART_RxCpltSemaphore, pdMS_TO_TICKS(UART_RX_TIMEOUT_MS)) == pdTRUE)
		{
			/* Извлекаем пакет */
			uint8_t *pData_rx = getPointToDataFromBuffer(&packet.length);
			if (pData_rx == NULL)
				continue;

			/* Копируем данные из приемного буфера */
			memcpy(packet.data, pData_rx, packet.length);
			/* Запись в очередь */
			xQueueSend(xDataRecievedQueue, &packet, pdMS_TO_TICKS(100));
		}
	}
	/* USER CODE END StartRxUARTTask */
}

/* USER CODE BEGIN Header_StartSendTelemetryTask */
/**
 * @brief Function implementing the SendTelemetryTa thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartSendTelemetryTask */
void StartSendTelemetryTask(void const *argument)
{
	/* USER CODE BEGIN StartSendTelemetryTask */
	static Telemetry telemetry = Telemetry_init_zero;
	static TelemetryPacket_t packet = {.bytes = {0}, .length = 0};
	static uint8_t protobuf_buffer[MAX_PAYLOAD_SIZE] = {0};
	/* Infinite loop */
	for (;;)
	{
		/*  Ожидаем запроса не телеметрию */
		xSemaphoreTake(xCMD_TelemetrySemaphore, portMAX_DELAY);

		/* Извлекаем структура данных телеметрии из очереди (не блокирующий вызов). Читаем БЕЗ УДАЛЕНИЯ элемента */
		xQueuePeek(xTelemetryToSendQueue, &telemetry, 0);

		/* Формирование пакета телеметрии */
		/* Protobuf */
		pb_ostream_t ostream = pb_ostream_from_buffer(protobuf_buffer, sizeof(protobuf_buffer));
		bool status = pb_encode(&ostream, Telemetry_fields, &telemetry);

		/* LL Protocol */
		if (status)
			packet.length = buildPacket(packet.bytes, protobuf_buffer, ostream.bytes_written, "GT");
		else
			packet.length = buildPacket(packet.bytes, "ERROR\r\n", strlen("ERROR\r\n"), "GT");

		if (startSendUART(packet.bytes, packet.length))
			/* Ожидаем завершения передачи */
			if (xSemaphoreTake(xUART_TxCpltSemaphore, pdMS_TO_TICKS(100)) == pdFALSE)
				__NOP();
	}
	/* USER CODE END StartSendTelemetryTask */
}

/* USER CODE BEGIN Header_StartThrustersControlTask */
/**
 * @brief Function implementing the ThrustersControlTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartThrustersControlTask */
void StartThrustersControlTask(void const *argument)
{
	/* USER CODE BEGIN StartThrustersControlTask */
	static ThrustersControl control = ThrustersControl_init_zero;

	thrustersInit();
	/* Infinite loop */
	for (;;)
	{
		xQueueReceive(xThrustersControlQueue, &control, portMAX_DELAY);

		switch (control.state)
		{
		case ThrustersState_THRUSTERS_POWER_ON:
			thrustersStop();
			thrustersPowerOn();
			osDelay(2000); // дать время ESC включиться
			break;
		case ThrustersState_THRUSTERS_POWER_OFF:
			thrustersStop();
			thrustersPowerOff();
			break;
		case ThrustersState_THRUSTERS_STOP:
			thrustersStop();
			break;
		case ThrustersState_THRUSTERS_NORMAL_OPERATION:
			thrustersControl(&control);
			break;
		default:
			break;
		}
	}
	/* USER CODE END StartThrustersControlTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
