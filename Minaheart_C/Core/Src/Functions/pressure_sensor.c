#include "main.h"
#include <stdio.h>   // Needed for snprintf() to format the text message
#include <math.h>

// Macro to pull the CS pin LOW (select the BMP580, starts an SPI transaction)
#define BMP580_CS_LOW()   HAL_GPIO_WritePin(BMP580_CS_GPIO_Port, BMP580_CS_Pin, GPIO_PIN_RESET)
// Macro to pull the CS pin HIGH (deselect the BMP580, ends the SPI transaction)
#define BMP580_CS_HIGH()  HAL_GPIO_WritePin(BMP580_CS_GPIO_Port, BMP580_CS_Pin, GPIO_PIN_SET)

uint8_t bmp580_read_reg(uint8_t reg, SPI_HandleTypeDef spi);

double BMP580_readPressure(SPI_HandleTypeDef spi){
	int32_t rawPress = 0;
	uint8_t buf[3]; //reads pressure from 3 registers (0x20-0x22)
	buf[0] = bmp580_read_reg(0x20, spi); //reads PRESS_DATA_XLSB (see BMP580 datasheet)
	buf[1] = bmp580_read_reg(0x21, spi); //reads PRESS_DATA_LSB (see BMP580 datasheet)
	buf[2] = bmp580_read_reg(0x22, spi); //reads PRESS_DATA_MSB (see BMP580 datasheet)
	rawPress = ((int32_t)buf[2] << 16) | ((int32_t)buf[1] << 8) | buf[0];
	if (rawPress & 0x800000) rawPress |= 0xFF000000;
	return (double)rawPress / 64.0; //Converts to raw pressure because raw pressure is 1/64 per count value
}

double pressure_to_altitude(double measuredPressure, double seaLevelPressure){
	return 44330.0 * (1.0 - pow(measuredPressure / seaLevelPressure, 0.1903)); //NEEDS EXPLANATION!
}

// Reads one register from the BMP580 over SPI and returns its value
uint8_t bmp580_read_reg(uint8_t reg, SPI_HandleTypeDef spi)
{
    uint8_t tx = reg | 0x80;   // Set bit 7 of the address byte to 1 = "read" command, R/W bit is the MSB that sends first
    uint8_t rx = 0;            // Variable to hold the byte received from the sensor

    BMP580_CS_LOW();                                   // Select the sensor (CS low)
    HAL_SPI_Transmit(&spi, &tx, 1, HAL_MAX_DELAY);   // Send the register address with the read flag
    HAL_SPI_Receive(&spi, &rx, 1, HAL_MAX_DELAY);    // Clock in the register value from the sensor
    BMP580_CS_HIGH();                                  // Deselect the sensor (CS high)
    return rx;                                         // Return the value that was read
}

void bmp580_write_reg(uint8_t reg, uint8_t value, SPI_HandleTypeDef *hspi) {
    uint8_t tx[2];
    tx[0] = reg & 0x7F; // Write command (Bit 7 = 0)
    tx[1] = value;      // Value to write

    BMP580_CS_LOW();
    HAL_SPI_Transmit(hspi, tx, 2, HAL_MAX_DELAY);
    BMP580_CS_HIGH();
}
