#include "main.h"


#define SD_CS_LOW()   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET)
#define SD_CS_HIGH()  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET)

// Send and receive a single byte over SPI
uint8_t SD_SPI_ReadWrite(uint8_t data, SPI_HandleTypeDef spi) {
    uint8_t rx_data;
    HAL_SPI_TransmitReceive(&spi, &data, &rx_data, 1, HAL_MAX_DELAY);
    return rx_data;
}

// Send an SD Command and return the R1 response byte
uint8_t SD_SendCmd(uint8_t cmd, uint32_t arg, uint8_t crc, SPI_HandleTypeDef spi) {
    uint8_t res;
    uint8_t retry = 0;

    // Deselect and select card with dummy bytes to synchronize
    SD_CS_HIGH();
    SD_SPI_ReadWrite(0xFF, spi);
    SD_CS_LOW();
    SD_SPI_ReadWrite(0xFF, spi);

    // Send Command Packet (6 Bytes)
    SD_SPI_ReadWrite(cmd | 0x40, spi);
    SD_SPI_ReadWrite((uint8_t)(arg >> 24), spi);
    SD_SPI_ReadWrite((uint8_t)(arg >> 16), spi);
    SD_SPI_ReadWrite((uint8_t)(arg >> 8), spi);
    SD_SPI_ReadWrite((uint8_t)arg, spi);
    SD_SPI_ReadWrite(crc, spi);

    // Wait for response (R1 byte has MSB = 0)
    do {
        res = SD_SPI_ReadWrite(0xFF, spi);
        retry++;
    } while ((res & 0x80) && (retry < 0xFF));

    return res;
}

uint8_t SD_Init(SPI_HandleTypeDef spi) {
    uint8_t res;
    uint16_t retry = 0;

    SD_CS_HIGH();

    // 1. Step 1: Send >74 clock cycles with CS High
    for (uint8_t i = 0; i < 10; i++) {
        SD_SPI_ReadWrite(0xFF, spi);
    }

    // 2. Step 2: Send CMD0 (Software Reset)
    res = SD_SendCmd(0, 0, 0x95, spi); // CRC 0x95 is mandatory for CMD0
    if (res != 0x01) {
        SD_CS_HIGH();
        return 1; // Error: Card didn't enter IDLE mode
    }

    // 3. Step 3: Send CMD8 (Check Interface Condition for SD v2.0+)
    res = SD_SendCmd(8, 0x000001AA, 0x87, spi); // CRC 0x87 is mandatory for CMD8
    if (res == 0x01) {
        // Read 4-byte response payload for CMD8
        for (uint8_t i = 0; i < 4; i++) {
            SD_SPI_ReadWrite(0xFF, spi);
        }
    }

    // 4. Step 4: Send ACMD41 (Initialize Card) until R1 == 0x00
    do {
        SD_SendCmd(55, 0, 0x01, spi);              // CMD55 (APP_CMD prefix)
        res = SD_SendCmd(41, 0x40000000, 0x01,spi); // ACMD41 with HCS bit set
        HAL_Delay(10);
        retry++;
    } while (res != 0x00 && retry < 1000);

    SD_CS_HIGH();
    SD_SPI_ReadWrite(0xFF, spi); // Extra clocks

    if (retry >= 1000) {
        return 2; // Timeout Error
    }

    return 0; // Success! Card is ready.
}

uint8_t SD_ReadSector(uint32_t sector, uint8_t *buffer, SPI_HandleTypeDef spi) {
    uint8_t res = SD_SendCmd(17, sector, 0x01, spi);
    if (res != 0x00) return 1; // Command rejected

    // Wait for Data Token (0xFE)
    uint16_t timeout = 10000;
    while (SD_SPI_ReadWrite(0xFF, spi) != 0xFE) {
        if (--timeout == 0) {
            SD_CS_HIGH();
            return 2; // Timeout
        }
    }

    // Read 512 Bytes
    for (uint16_t i = 0; i < 512; i++) {
        buffer[i] = SD_SPI_ReadWrite(0xFF, spi);
    }

    // Ignore 2-Byte CRC
    SD_SPI_ReadWrite(0xFF, spi);
    SD_SPI_ReadWrite(0xFF, spi);

    SD_CS_HIGH();
    SD_SPI_ReadWrite(0xFF, spi);
    return 0; // Read success
}

uint8_t SD_WriteSector(uint32_t sector, uint8_t *buffer, SPI_HandleTypeDef spi) {
    // 1. Send CMD24 (WRITE_BLOCK)
    uint8_t res = SD_SendCmd(24, sector, 0x01, spi);
    if (res != 0x00) return 1; // Command rejected

    // 2. Send Start Block Token (0xFE)
    SD_SPI_ReadWrite(0xFE, spi);

    // 3. Transmit 512 Bytes of Data
    for (uint16_t i = 0; i < 512; i++) {
        SD_SPI_ReadWrite(buffer[i], spi);
    }

    // 4. Send 2 Dummy CRC Bytes
    SD_SPI_ReadWrite(0xFF, spi);
    SD_SPI_ReadWrite(0xFF, spi);

    // 5. Read Data Response Token
    // Mask with 0x1F to isolate the 3-bit status: (0x05 = Data Accepted)
    uint8_t response = SD_SPI_ReadWrite(0xFF, spi);
    if ((response & 0x1F) != 0x05) {
        SD_CS_HIGH();
        return 2; // Write rejected by card
    }

    // 6. Wait while the SD card is busy writing (MISO pulled LOW)
    uint32_t timeout = 500000;
    while (SD_SPI_ReadWrite(0xFF, spi) == 0x00) {
        if (--timeout == 0) {
            SD_CS_HIGH();
            return 3; // Timeout waiting for busy release
        }
    }

    SD_CS_HIGH();
    SD_SPI_ReadWrite(0xFF, spi); // Extra clock cycles
    return 0; // Success
}
