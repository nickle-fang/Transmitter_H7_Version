/**
  ******************************************************************************
  * @file    LwIP/LwIP_HTTP_Server_Netconn_RTOS/Src/main.c 
  * @author  MCD Application Team
  * @brief   This sample code implements a http server application based on 
  *          Netconn API of LwIP stack and FreeRTOS. This application uses 
  *          STM32H7xx the ETH HAL API to transmit and receive data. 
  *          The communication is done with a web browser of a remote PC.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "NRF24L01.h"
#include "UDP_SERVER.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

SPI_HandleTypeDef hspi4;
SPI_HandleTypeDef hspi5;

UART_HandleTypeDef huart3;

uint8_t TX_frequency = 0x5e;			//NRF24L01发射频率   6号频点时为0x18， 8号频点时为0x5a，
uint8_t RX_frequency = 0x5e;			//NRF24L01接收频率
uint8_t tx_buf[25];
uint8_t rx_buf[25];



double TX_Num = 1;
double Rx_Num = 1;
double send_rate;
uint8_t print_out[200];
uint8_t all[4] = "100%";

struct netif gnetif; /* network interface structure */

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void BSP_Config(void);
static void StartThread(void const * argument);
static void Netif_Config(void);
static void MPU_Config(void);
static void CPU_CACHE_Enable(void);
static void MX_USART3_UART_Init(void);

/* Private functions ---------------------------------------------------------*/
//任务函数定义
static void HeartLedThread(void const * argument);
static void UDPThread(void const * argument);
static void RXThread(void const * argument);

//输出端口初始化函数定义	
void MX_SPI4_Init(void);
void MX_SPI5_Init(void);
static void MX_GPIO_Init(void);

//2401相关参数
extern unsigned char tmp_buf[25];


/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  /* Configure the MPU attributes as Device memory for ETH DMA descriptors */
  MPU_Config();
  
  /* Enable the CPU Cache */
  CPU_CACHE_Enable();

  /* STM32H7xx HAL library initialization:
       - Configure the TIM6 to generate an interrupt each 1 msec
       - Set NVIC Group Priority to 4
       - Low Level Initialization
     */
  HAL_Init();  
	
	
  /* Configure the system clock to 400 MHz */
  SystemClock_Config(); 
  
	//初始化SPI4，SPI5端口
	MX_GPIO_Init();
	MX_SPI4_Init();
  MX_SPI5_Init();
	
	//MX_USART3_UART_Init();
	
  /* Configure the LCD ...*/
  BSP_Config();

	while(NRF24L01_TX_Check())    //发射2401不在位
	{
		BSP_LED_Toggle(LED3);
		HAL_Delay(1000);
	}
	
	while(NRF24L01_RX_Check())    //接收2401不在位
	{
		BSP_LED_Toggle(LED3);
		HAL_Delay(1000);
	}
  
  /* Init thread */
  osThreadDef(Start, StartThread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE * 4);
  osThreadCreate (osThread(Start), NULL);
	
	//led 闪烁子程序  by fxz //////////////////////////////////////////////////////////////////////////////////////////////fxz
	osThreadDef(Heart_Led, HeartLedThread, osPriorityIdle, 0, configMINIMAL_STACK_SIZE * 4);
  osThreadCreate (osThread(Heart_Led), NULL);
  
  /* Start scheduler */
  osKernelStart();
  
  /* We should never get here as control is now taken by the scheduler */
  for( ;; );
}



/**
  * @brief  Start Led Thread ///////////fxz
  * @param  argument not used
  * @retval None
  */
static void HeartLedThread(void const * argument)
{
	while (1)
	{
		BSP_LED_Toggle(LED2);
		HAL_GPIO_TogglePin(LED_HEART_GPIO_Port, LED_HEART_Pin);
		osDelay(1000);   //延时1000ms
		/*-------------------------------------------------------------------------------for test
		---------------------------------------------------------------------------------*/
//		send_rate = Rx_Num / TX_Num;
//		if(send_rate < 1)
//		{
////			print_out[0] = 0x30 + (int)(send_rate * 10);
////			print_out[1] = 0x30 + (int)(send_rate * 100) % 10;
////			print_out[2] = 0x30 + (int)(send_rate * 1000) % 10;
////			print_out[3] = 0x30 + (int)(send_rate * 10000) % 10;
////			print_out[4] = 0x30 + (int)(send_rate * 100000) % 10;
////			HAL_UART_Transmit(&huart3, print_out, 5, 0xffff);
//			sprintf(print_out, "%10.10f", send_rate);
//			HAL_UART_Transmit(&huart3, print_out, 10, 0xffff);
//		}
//		else
//			HAL_UART_Transmit(&huart3, all, 4, 0xffff);
//		
//		TX_Num = Rx_Num = 1;
		/*-------------------------------------------------------------------------------
		---------------------------------------------------------------------------------*/
		//osDelay(10000);  //延时1000ms
	}
}

/**
  * @brief  Start UDPThread //////fxz
  * @param  argument not used
  * @retval None
  */
static void UDPThread(void const * argument)
{
		//__IO uint32_t LocalTime = 0; /* this variable is used to create a time reference incremented by 10ms */
		/* configure ethernet (GPIOs, clocks, MAC, DMA) */ 
		//ETH_BSP_Config();

		/* UDP_server Init */
		UDP_server_init();
		//udp_Multicast();   //组播程序初始化
		//while(1)
		//{
	  osDelay(2500);//挂起2500ms，以便其他线程运行
		//}
		osThreadTerminate(NULL);
}

int package_receive = 0;
int i = 0;
static void RXThread(void const * argument)
{
  while(1)
	{
		if(NRF24L01_RxPacket(rx_buf) == 0)   //收到数据
		{
			package_receive ++;
			if (package_receive == 32)
			{
				BSP_LED_Toggle(LED1);
				HAL_GPIO_TogglePin(LED_RX_GPIO_Port, LED_RX_Pin);
				package_receive = 0;
			}
			
			 
//			for (i = 0; i <= 24; i++)
//			{
//				if (rx_buf[i] != 0xff)
//				{
//					break;
//				}
//			}
//			if(i != 25)
//			{
				udp_demo_senddata();
				//Rx_Num ++;                    /////////////////////////////////////////////////////////////////////////////for test
//			}
			
		}
		osDelay(1);//挂起1ms，以便其他线程运行
	}
}



/**
  * @brief  Start Thread 
  * @param  argument not used
  * @retval None
  */
static void StartThread(void const * argument)
{   
  /* Create tcp_ip stack thread */
  tcpip_init(NULL, NULL);
  
  /* Initialize the LwIP stack */
  Netif_Config();

  /* Initialize webserver demo */
  //http_server_netconn_init();
  
	
  //udp服务器子程序  by fxz//////////////////////////////////////////////////////////////////////////////////////////////////fxz
  osThreadDef(UDP_Start, UDPThread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE * 4);
  osThreadCreate (osThread(UDP_Start), NULL);

	
	//24L01接收程序  by fxz ///////////////////////////////////////////////////////////////////////////////////////////////////fxz
	osThreadDef(RX24L01, RXThread, osPriorityRealtime, 0, configMINIMAL_STACK_SIZE * 4);
  osThreadCreate (osThread(RX24L01), NULL);
	
  for( ;; )
  {
    /* Delete the Init Thread */ 
    osThreadTerminate(NULL);
  }
}

/**
  * @brief  BSP Configuration 
  * @param  None
  * @retval None
  */
static void BSP_Config(void)
{
  BSP_LED_Init(LED1);    //闪烁：接收成功
	BSP_LED_Init(LED2);    //常亮：网线已经插好, 正常状态慢闪烁
  BSP_LED_Init(LED3);    //闪烁：发送成功
}

/**
  * @brief SPI4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI4_Init(void)
{

  /* USER CODE BEGIN SPI4_Init 0 */

  /* USER CODE END SPI4_Init 0 */

  /* USER CODE BEGIN SPI4_Init 1 */

  /* USER CODE END SPI4_Init 1 */
  /* SPI4 parameter configuration*/
  hspi4.Instance = SPI4;
  hspi4.Init.Mode = SPI_MODE_MASTER;
  hspi4.Init.Direction = SPI_DIRECTION_2LINES;
  hspi4.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi4.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi4.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi4.Init.NSS = SPI_NSS_SOFT;
  hspi4.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;  // 8Mbps
  hspi4.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi4.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi4.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi4.Init.CRCPolynomial = 7;
  hspi4.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  hspi4.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi4.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi4.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi4.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi4.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi4.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi4.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi4.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi4.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  if (HAL_SPI_Init(&hspi4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI4_Init 2 */

  /* USER CODE END SPI4_Init 2 */

}

/**
  * @brief SPI5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI5_Init(void)
{

  /* USER CODE BEGIN SPI5_Init 0 */

  /* USER CODE END SPI5_Init 0 */

  /* USER CODE BEGIN SPI5_Init 1 */

  /* USER CODE END SPI5_Init 1 */
  /* SPI5 parameter configuration*/
  hspi5.Instance = SPI5;
  hspi5.Init.Mode = SPI_MODE_MASTER;
  hspi5.Init.Direction = SPI_DIRECTION_2LINES;
  hspi5.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi5.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi5.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi5.Init.NSS = SPI_NSS_SOFT;
  hspi5.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi5.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi5.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi5.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi5.Init.CRCPolynomial = 7;
  hspi5.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  hspi5.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi5.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi5.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi5.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi5.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi5.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi5.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi5.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi5.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  if (HAL_SPI_Init(&hspi5) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI5_Init 2 */

  /* USER CODE END SPI5_Init 2 */

}


static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOF, SPI4CSN_Pin|SPI4CE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOG, SPI5CSN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SPI5CE_GPIO_Port, SPI5CE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : SPI4CSN_Pin SPI4CE_Pin */
  GPIO_InitStruct.Pin = SPI4CSN_Pin|SPI4CE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI5CSN_Pin */
  GPIO_InitStruct.Pin = SPI5CSN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(SPI5CSN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : SPI5IRQ_Pin SPI4IRQ_Pin */
  GPIO_InitStruct.Pin = SPI5IRQ_Pin|SPI4IRQ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI5CE_Pin */
  GPIO_InitStruct.Pin = SPI5CE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(SPI5CE_GPIO_Port, &GPIO_InitStruct);
	
	/*Configure GPIO pin : LED_TX */
  GPIO_InitStruct.Pin = LED_TX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(LED_TX_GPIO_Port, &GPIO_InitStruct);
	
	/*Configure GPIO pin : LED_RX HEART */
  GPIO_InitStruct.Pin = LED_RX_Pin | LED_HEART_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(LED_RX_GPIO_Port, &GPIO_InitStruct);
	
	/*Configure GPIO pin : LED*/
  GPIO_InitStruct.Pin = LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

}


/**
  * @brief  Initializes the lwIP stack
  * @param  None
  * @retval None
  */
static void Netif_Config(void)
{
  ip_addr_t ipaddr;
  ip_addr_t netmask;
  ip_addr_t gw;
 
#if LWIP_DHCP
  ip_addr_set_zero_ip4(&ipaddr);
  ip_addr_set_zero_ip4(&netmask);
  ip_addr_set_zero_ip4(&gw);
#else
  IP_ADDR4(&ipaddr,IP_ADDR0,IP_ADDR1,IP_ADDR2,IP_ADDR3);
  IP_ADDR4(&netmask,NETMASK_ADDR0,NETMASK_ADDR1,NETMASK_ADDR2,NETMASK_ADDR3);
  IP_ADDR4(&gw,GW_ADDR0,GW_ADDR1,GW_ADDR2,GW_ADDR3);
#endif /* LWIP_DHCP */
  
  /* add the network interface */    
  netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);
  
  /*  Registers the default network interface. */
  netif_set_default(&gnetif);
  
  ethernet_link_status_updated(&gnetif); 
  
#if LWIP_NETIF_LINK_CALLBACK 
  netif_set_link_callback(&gnetif, ethernet_link_status_updated);
  
  osThreadDef(EthLink, ethernet_link_thread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE *2);
  osThreadCreate (osThread(EthLink), &gnetif);
#endif   
 
#if LWIP_DHCP
  /* Start DHCPClient */
  osThreadDef(DHCP, DHCP_Thread, osPriorityBelowNormal, 0, configMINIMAL_STACK_SIZE * 2);
  osThreadCreate (osThread(DHCP), &gnetif);
#endif 
}

///**
//  * @brief  System Clock Configuration
//  *         The system Clock is configured as follow : 
//  *            System Clock source            = PLL (HSE BYPASS)
//  *            SYSCLK(Hz)                     = 400000000 (CPU Clock)
//  *            HCLK(Hz)                       = 200000000 (AXI and AHBs Clock)
//  *            AHB Prescaler                  = 2
//  *            D1 APB3 Prescaler              = 2 (APB3 Clock  100MHz)
//  *            D2 APB1 Prescaler              = 2 (APB1 Clock  100MHz)
//  *            D2 APB2 Prescaler              = 2 (APB2 Clock  100MHz)
//  *            D3 APB4 Prescaler              = 2 (APB4 Clock  100MHz)
//  *            HSE Frequency(Hz)              = 8000000
//  *            PLL_M                          = 4
//  *            PLL_N                          = 400
//  *            PLL_P                          = 2
//  *            PLL_Q                          = 4
//  *            PLL_R                          = 2
//  *            VDD(V)                         = 3.3
//  *            Flash Latency(WS)              = 4
//  * @param  None
//  * @retval None
//  */
//static void SystemClock_Config(void)
//{
//  RCC_ClkInitTypeDef RCC_ClkInitStruct;
//  RCC_OscInitTypeDef RCC_OscInitStruct;
//	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
//  HAL_StatusTypeDef ret = HAL_OK;
//  
//  /*!< Supply configuration update enable */
//  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

//  /* The voltage scaling allows optimizing the power consumption when the device is
//     clocked below the maximum system frequency, to update the voltage scaling value
//     regarding system frequency refer to product datasheet.  */
//  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

//  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
//  
//  /* Enable D2 domain SRAM3 Clock (0x30040000 AXI)*/
//  __HAL_RCC_D2SRAM3_CLK_ENABLE();
//  
//  /* Enable HSE Oscillator and activate PLL with HSE as source */
//  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
//  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
//  RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
//  RCC_OscInitStruct.CSIState = RCC_CSI_OFF;
//  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
//  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

//  RCC_OscInitStruct.PLL.PLLM = 4;
//  RCC_OscInitStruct.PLL.PLLN = 400;
//  RCC_OscInitStruct.PLL.PLLFRACN = 0;
//  RCC_OscInitStruct.PLL.PLLP = 2;
//  RCC_OscInitStruct.PLL.PLLR = 2;
//  RCC_OscInitStruct.PLL.PLLQ = 4;

//  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
//  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
//  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
//  if(ret != HAL_OK)
//  {
//    while(1);
//  }
//  
//  /* Select PLL as system clock source and configure  bus clocks dividers */
//  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_D1PCLK1 | RCC_CLOCKTYPE_PCLK1 | \
//                                 RCC_CLOCKTYPE_PCLK2  | RCC_CLOCKTYPE_D3PCLK1);

//  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
//  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
//  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
//  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;  
//  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2; 
//  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2; 
//  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2; 
//  ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
//	
//  PeriphClkInitStruct.PeriphClockSelection =  RCC_PERIPHCLK_SPI45;
//  PeriphClkInitStruct.Spi45ClockSelection = RCC_SPI45CLKSOURCE_D2PCLK1;
//  if(ret != HAL_OK)
//  {
//    while(1);
//  }
//}


/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart3, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart3, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Supply configuration update enable 
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
  /** Configure the main internal regulator output voltage 
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 400;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_1;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART3|RCC_PERIPHCLK_SPI5
                              |RCC_PERIPHCLK_SPI4|RCC_PERIPHCLK_USB;
  PeriphClkInitStruct.Spi45ClockSelection = RCC_SPI45CLKSOURCE_D2PCLK1;
  PeriphClkInitStruct.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_D2PCLK1;
  //PeriphClkInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Enable USB Voltage detector 
  */
  HAL_PWREx_EnableUSBVoltageDetector();
}

/**
  * @brief  Configure the MPU attributes 
  * @param  None
  * @retval None
  */
static void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct;
  
  /* Disable the MPU */
  HAL_MPU_Disable();

  /* Configure the MPU attributes as Device not cacheable 
     for ETH DMA descriptors */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0x30040000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_256B;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  
  /* Configure the MPU attributes as Cacheable write through 
     for LwIP RAM heap which contains the Tx buffers */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0x30044000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_16KB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER1;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Enable the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

/**
  * @brief  CPU L1-Cache enable.
  * @param  None
  * @retval None
  */
static void CPU_CACHE_Enable(void)
{
  /* Enable I-Cache */
  SCB_EnableICache();

  /* Enable D-Cache */
  SCB_EnableDCache();
}

void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
