#include "main.h"
#include <stdio.h>   // Needed for snprintf() to format the text message
#include "Functions/pressure_sensor.h"
#include "Functions/telemetry_debug.h"
#include "Functions/SD_card.h"
#include <string.h>
#include <stdbool.h>
#include "fatfs.h"
#include <stdarg.h>

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
void myprintf(const char *fmt, ...);

void myprintf(const char *fmt, ...) {
  static char buffer[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  int len = strlen(buffer);
  HAL_UART_Transmit(&huart1, (uint8_t*)buffer, len, HAL_MAX_DELAY);

}

// Reads one register from the BMP580 over SPI and returns its value
/* USER CODE END PV */

char buffer_main[64];
uint8_t writeBuf[512];
uint8_t readBuf[512];

// Custom log messages
char msg_init[] = "\r\n=== USART1 SD Card Test Output ===\r\n\n";
char msg_read_prefix[] = "Read Result: \"";
char msg_quote_newline[] = "\"\r\n";

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_SPI1_Init(void);
static void MX_SPI2_Init(void);

bool isLaunched;

int main(void)
{
	HAL_Init();
	SystemClock_Config();
	MX_GPIO_Init();
	MX_USART1_UART_Init();
	MX_SPI1_Init();
	MX_SPI2_Init();
	MX_FATFS_Init(); //DO NOT FORGET TO INITIALIZE THE FATFS! OR ELSE IT'LL HANG!

    //BMP setup
    HAL_Delay(10);                       		// Wait 10 ms to let the sensor finish powering up
    bmp580_read_reg(0x01, hspi1);               // Dummy read of CHIP_ID: toggles CS low so the sensor locks into SPI mode (result ignored)
    bmp580_write_reg(0x31, 0x02, &hspi1);		// IIR filter coefficient set to 3 (Balanced response rate vs noise rejection)
    bmp580_write_reg(0x36, 0x7B, &hspi1); 		// Enable pressure, OSR pressure x128, OSR temperature x8
    bmp580_write_reg(0x37, 0x03, &hspi1); 		// Run in continuous sampling mode
    HAL_Delay(10);

    //SD TEST
    myprintf("\r\n~ SD card demo by kiwih ~\r\n\r\n");

      HAL_Delay(1000); //a short delay is important to let the SD card settle

      //some variables for FatFs
      FATFS FatFs; 	//Fatfs handle
      FIL fil; 		//File handle
      FRESULT fres; //Result after operations

      //Open the file system
      fres = f_mount(&FatFs, "", 1); //1=mount now
      if (fres != FR_OK) {
    	myprintf("f_mount error (%i)\r\n", fres);
    	while(1);
      }

      //Let's get some statistics from the SD card
      DWORD free_clusters, free_sectors, total_sectors;

      FATFS* getFreeFs;

      fres = f_getfree("", &free_clusters, &getFreeFs);
      if (fres != FR_OK) {
    	myprintf("f_getfree error (%i)\r\n", fres);
    	while(1);
      }

      //Formula comes from ChaN's documentation
      total_sectors = (getFreeFs->n_fatent - 2) * getFreeFs->csize;
      free_sectors = free_clusters * getFreeFs->csize;

      myprintf("SD card stats:\r\n%10lu KiB total drive space.\r\n%10lu KiB available.\r\n", total_sectors / 2, free_sectors / 2);

      //Now let's try to open file "test.txt"
      fres = f_open(&fil, "test.txt", FA_READ);
      if (fres != FR_OK) {
    	myprintf("f_open error (%i)\r\n");
    	while(1);
      }
      myprintf("I was able to open 'test.txt' for reading!\r\n");

      //Read 30 bytes from "test.txt" on the SD card
      BYTE readBuf[30];

      //We can either use f_read OR f_gets to get data out of files
      //f_gets is a wrapper on f_read that does some string formatting for us
      TCHAR* rres = f_gets((TCHAR*)readBuf, 30, &fil);
      if(rres != 0) {
    	myprintf("Read string from 'test.txt' contents: %s\r\n", readBuf);
      } else {
    	myprintf("f_gets error (%i)\r\n", fres);
      }

      //Be a tidy kiwi - don't forget to close your file!
      f_close(&fil);

      //Now let's try and write a file "write.txt"
      fres = f_open(&fil, "write.txt", FA_WRITE | FA_OPEN_ALWAYS | FA_CREATE_ALWAYS);
      if(fres == FR_OK) {
    	myprintf("I was able to open 'write.txt' for writing\r\n");
      } else {
    	myprintf("f_open error (%i)\r\n", fres);
      }

      //Copy in a string
      strncpy((char*)readBuf, "a new file is made!", 19);
      UINT bytesWrote;
      fres = f_write(&fil, readBuf, 19, &bytesWrote);
      if(fres == FR_OK) {
    	myprintf("Wrote %i bytes to 'write.txt'!\r\n", bytesWrote);
      } else {
    	myprintf("f_write error (%i)\r\n");
      }

      //Be a tidy kiwi - don't forget to close your file!
      f_close(&fil);

      //We're done, so de-mount the drive
      f_mount(NULL, "", 0);

    //Setup the start pressure for starting altitude
    double initialPressure = BMP580_readPressure(hspi1);
    //Inital print
    snprintf(buffer_main, sizeof(buffer_main), "-----------------------SENSOR START!-----------------------\n");
    HAL_UART_Transmit(&huart1, (uint8_t*)buffer_main, sizeof(buffer_main), 100);


  while (1)
  {
	  if ((HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_RESET) && !isLaunched){
		  isLaunched = 1;
	  }
	  double rawPressure = BMP580_readPressure(hspi1);	// 1. Read the raw pressure
	  double altitude = pressure_to_altitude(rawPressure, initialPressure); // 2. Calculate raw altitude into altitude
	  // 3. Apply the dynamic offset to eliminate sensor start-up drift errors
	  // 4. Flash Logging
	  serial_result_display(huart1, rawPressure, altitude); // 5. Telemetry Diagnostics
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 72;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BMP580_CS_GPIO_Port, BMP580_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : BMP580_CS_Pin */
  GPIO_InitStruct.Pin = BMP580_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BMP580_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SD_CS_Pin */
  GPIO_InitStruct.Pin = SD_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SD_CS_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
