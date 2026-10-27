#ifndef pressure_sensor
#define pressure_sensor

double BMP580_readPressure();
double pressure_to_altitude(double measuredPressure, double seaLevelPressure);

// Macro to pull the CS pin LOW (select the BMP580, starts an SPI transaction)
#define BMP580_CS_LOW()   HAL_GPIO_WritePin(BMP580_CS_GPIO_Port, BMP580_CS_Pin, GPIO_PIN_RESET)

// Macro to pull the CS pin HIGH (deselect the BMP580, ends the SPI transaction)
#define BMP580_CS_HIGH()  HAL_GPIO_WritePin(BMP580_CS_GPIO_Port, BMP580_CS_Pin, GPIO_PIN_SET)

uint8_t bmp580_read_reg(uint8_t reg, SPI_HandleTypeDef spi);
void bmp580_write_reg(uint8_t reg, uint8_t value, SPI_HandleTypeDef *hspi);

#endif
