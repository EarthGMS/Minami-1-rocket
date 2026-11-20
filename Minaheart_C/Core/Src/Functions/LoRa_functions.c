#include "main.h"
#include "LoRa_functions.h"
#include <string.h>

/* Dictionary
 * FIFO - First-in-First-out
 * Preamble - introductory signal
 * LNA - Low noise amplifier
 * PA - Power amplifier
 * CRC - Cyclic Redundancy Check
 * */

/* Chip select pin (PB1) */
#define CS_PORT    GPIOB
#define CS_PIN     GPIO_PIN_1

/* Reset pin (PB0) */
#define RST_PORT   GPIOB
#define RST_PIN    GPIO_PIN_0

/* SX1278 LoRa registers (REF P.90 of SX1278 document*/
#define REG_FIFO              0x00 //First-in First-out register
#define REG_OP_MODE           0x01 //OP mode (use to change modes) IMPORTANT
#define REG_FRF_MSB           0x06 //RF carrier frequency MSB
#define REG_FRF_MID           0x07 //RF carrier frequency MID
#define REG_FRF_LSB           0x08 //RF carrier frequency LSB
#define REG_PA_CONFIG         0x09 //PA selection and output power control
#define REG_LNA               0x0C //LNA settings
#define REG_FIFO_ADDR_PTR     0x0D //FIFO SPI pointer
#define REG_FIFO_TX_BASE      0x0E //Start TX data
#define REG_FIFO_RX_BASE      0x0F //Start RX data
#define REG_FIFO_RX_CURRENT   0x10 //Start address of the last packet received
#define REG_IRQ_FLAGS        0x12  //Interrupt Request Flags
#define REG_RX_NB_BYTES      0x13  //Number of bytes received
#define REG_MODEM_CONFIG1     0x1D //Modem PHY config 1
#define REG_MODEM_CONFIG2     0x1E //Modem PHY config 2
#define REG_PREAMBLE_MSB      0x20 //Size of the preamble
#define REG_PREAMBLE_LSB      0x21 //Size of the preamble
#define REG_PAYLOAD_LENGTH    0x22 //LoRa payload length*
#define REG_MODEM_CONFIG3     0x26 //Modem PHY config 3
#define REG_SYNC_WORD         0x39 //LoRa sync word*
#define REG_DIO_MAPPING1      0x40 //Pin DI0-DI3 (we use DI0 so we use only MAPPING1)
#define REG_VERSION           0x42 //Module ID

/* SX1278 operating modes, LF bit enabled */ /*This requires audit*/
#define MODE_SLEEP          0x88 //Sleep mode
#define MODE_STDBY          0x89 //Standby mode
#define MODE_FSTX           0x8A //Frequency synthesis (FS) TX
#define MODE_TX             0x8B //Transmit mode
#define MODE_FSRX           0x8C //FS RX
#define MODE_RXCONTINUOUS   0x8D //Receive mode
#define MODE_RXSINGLE       0x8E //Receive single
#define MODE_CAD            0x8F //Channel connectivity detection (CAD)

#define IRQ_RX_DONE          0x40  //Interrupt receive done
#define IRQ_CRC_ERROR        0x20  //Interrupt CRC error
#define IRQ_TX_DONE          0x08  //Interrupt transmit done

extern SPI_HandleTypeDef hspi3; //use extern to define hspi3 is defined elsewhere

static void cs_low(void) //Set CS LOW
{
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
}

static void cs_high(void) //Set CS HIGH
{
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
}

static uint8_t read_reg(uint8_t addr) //write data (TBD!)
{
	uint8_t tx[2] = { (uint8_t)(addr & 0x7F), 0 }; //Set addr bit 7 to 0, while data to 0 (because it's read)
	uint8_t rx[2] = { 0 };

	cs_low();
	HAL_StatusTypeDef status =HAL_SPI_TransmitReceive(&hspi3, tx, rx, 2, 100); //SPI RX
	cs_high();

	return (status == HAL_OK) ? rx[1] : 0xFF; //If the result is OK, send data, if not, send 0xFF (ERROR)
}

static HAL_StatusTypeDef write_reg(uint8_t addr, uint8_t value) //Write data
{
	uint8_t tx[2] = {
		(uint8_t)(addr | 0x80), value //Set up tx mask
	};

	cs_low();
	HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi3, tx, 2, 100); //SPI TX
	cs_high();

	return status; //Return status
}

static HAL_StatusTypeDef write_fifo( //Write FIFO
    const uint8_t *data, uint8_t length)
{
	uint8_t command = REG_FIFO | 0x80;

	cs_low();
	HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi3, &command, 1, 100);

	if (status == HAL_OK)
		status = HAL_SPI_Transmit(&hspi3, (uint8_t *)data, length, 100);

	cs_high();
	return status;
}

static HAL_StatusTypeDef read_fifo( //read FIFO
    uint8_t *data, uint8_t length)
{
	uint8_t command = REG_FIFO & 0x7F;

	    cs_low();
	    HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi3, &command, 1, 100);

	    if (status == HAL_OK)
	        status = HAL_SPI_Receive(&hspi3, data, length, 100);

	    cs_high();
	    return status;
}

uint8_t LoRa_ReadVersion(void) //read version (TBD!)
{
	return read_reg(REG_VERSION);
}


LoRa_Status LoRa_Init(void)
{
	/* Hardware reset */
	cs_high();
	HAL_GPIO_WritePin(RST_PORT, RST_PIN, GPIO_PIN_RESET);
	HAL_Delay(2);
	HAL_GPIO_WritePin(RST_PORT, RST_PIN, GPIO_PIN_SET);
	HAL_Delay(10);

	/* Check if it's the right version (0x12) if not, return LORA_ERROR */
	if (LoRa_ReadVersion() != 0x12)
		return LORA_ERROR;

	//Set to sleep mode
	if (write_reg(REG_OP_MODE, 0x08) != HAL_OK) //Delete this
	        return LORA_ERROR;
	    HAL_Delay(2);

	if (write_reg(REG_OP_MODE, MODE_SLEEP) != HAL_OK)
			return LORA_ERROR;
		HAL_Delay(2);

	/* Frequency = 433 MHz
		   FRF = frequency * 2^19 / 32 MHz */
		uint64_t frf =
			((uint64_t)LORA_FREQ_HZ << 19) / 32000000ULL; //Needs audit

	write_reg(REG_FRF_MSB, (uint8_t)(frf >> 16)); //Needs audit as well
	write_reg(REG_FRF_MID, (uint8_t)(frf >> 8));
	write_reg(REG_FRF_LSB, (uint8_t)frf);

	//SETUP PART

	/* TX power: PA_BOOST, nominal +17 dBm */
	write_reg(REG_PA_CONFIG, 0x8F);

	/* RX LNA boost */
	write_reg(REG_LNA, read_reg(REG_LNA) | 0x03);

	/* BW 125 kHz, CR 4/5, explicit header */
	write_reg(REG_MODEM_CONFIG1, 0x72);

	/* SF7, CRC enabled */
	write_reg(REG_MODEM_CONFIG2, 0x74);

	/* AGC auto, low data rate optimize off */
	write_reg(REG_MODEM_CONFIG3, 0x04);

	/* Preamble: 8 symbols */
	write_reg(REG_PREAMBLE_MSB, 0x00);
	write_reg(REG_PREAMBLE_LSB, 0x08);

	/* Private sync word */
	write_reg(REG_SYNC_WORD, 0x12); //Needs revision after the project

	/* FIFO addresses */
	write_reg(REG_FIFO_TX_BASE, 0x00);
	write_reg(REG_FIFO_RX_BASE, 0x00);

	/* DIO0 mapping: RxDone in RX, TxDone in TX */
	write_reg(REG_DIO_MAPPING1, 0x00);

	write_reg(REG_IRQ_FLAGS, 0xFF);
	write_reg(REG_OP_MODE, MODE_STDBY);
	HAL_Delay(2);

	//END OF THE SETUP PART

	return (read_reg(REG_OP_MODE) == MODE_STDBY)
		? LORA_OK : LORA_ERROR;
}

void LoRa_StartReceive(void)
{
	write_reg(REG_OP_MODE, MODE_STDBY);
	write_reg(REG_FIFO_ADDR_PTR, 0x00);
	write_reg(REG_IRQ_FLAGS, 0xFF);
	write_reg(REG_OP_MODE, MODE_RXCONTINUOUS);
}

void LoRa_receiveConfigData(void){

}

LoRa_Status LoRa_Send(const uint8_t *data,
                      uint8_t length,
                      uint32_t timeout_ms)
{
	if (data == NULL || length == 0)
	        return LORA_ERROR;

	write_reg(REG_OP_MODE, MODE_STDBY);
	write_reg(REG_IRQ_FLAGS, 0xFF);
	write_reg(REG_FIFO_ADDR_PTR, 0x00);
	write_reg(REG_PAYLOAD_LENGTH, length);

	if (write_fifo(data, length) != HAL_OK)
		return LORA_ERROR;

	write_reg(REG_OP_MODE, MODE_TX);

	uint32_t start = HAL_GetTick();

	while (!(read_reg(REG_IRQ_FLAGS) & IRQ_TX_DONE))
	{
		if ((uint32_t)(HAL_GetTick() - start)
			>= timeout_ms)
		{
			write_reg(REG_OP_MODE, MODE_STDBY);
			return LORA_TIMEOUT;
		}
	}

	write_reg(REG_IRQ_FLAGS, IRQ_TX_DONE);
	write_reg(REG_OP_MODE, MODE_STDBY);

	return LORA_OK;
}

LoRa_Status LoRa_Receive(uint8_t *data,
                         uint8_t capacity,
                         uint8_t *length)
{
	if (data == NULL || length == NULL)
	        return LORA_ERROR;

	*length = 0; //This requires audit

	uint8_t flags = read_reg(REG_IRQ_FLAGS);

	if (!(flags & IRQ_RX_DONE))
		return LORA_NO_PACKET;

	if (flags & IRQ_CRC_ERROR)
	{
		write_reg(REG_IRQ_FLAGS, 0xFF);
		return LORA_CRC_ERROR;
	}

	uint8_t received = read_reg(REG_RX_NB_BYTES);

	if (received > capacity)
	{
		write_reg(REG_IRQ_FLAGS, 0xFF);
		return LORA_ERROR;
	}

	uint8_t address = read_reg(REG_FIFO_RX_CURRENT);
	write_reg(REG_FIFO_ADDR_PTR, address);

	if (read_fifo(data, received) != HAL_OK)
		return LORA_ERROR;

	*length = received;
	write_reg(REG_IRQ_FLAGS, 0xFF);

	return LORA_OK;
}
