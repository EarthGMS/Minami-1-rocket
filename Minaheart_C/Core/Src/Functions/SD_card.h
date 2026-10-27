#ifndef SD_card
#define SD_card

uint8_t SD_SPI_ReadWrite(uint8_t data, SPI_HandleTypeDef spi);
uint8_t SD_SendCmd(uint8_t cmd, uint32_t arg, uint8_t crc, SPI_HandleTypeDef spi);
uint8_t SD_Init(SPI_HandleTypeDef spi);
uint8_t SD_ReadSector(uint32_t sector, uint8_t *buffer, SPI_HandleTypeDef spi);
uint8_t SD_WriteSector(uint32_t sector, uint8_t *buffer, SPI_HandleTypeDef spi);
#endif
