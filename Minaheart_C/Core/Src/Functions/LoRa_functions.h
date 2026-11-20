#ifndef LoRa_func
#define LoRa_func

#include "main.h"
#include <stdint.h>

#define LORA_FREQ_HZ 433000000UL //Set LoRa at 433MHz unsigned long (legal restrictions at 433MHz)

typedef enum {
	LORA_OK = 0,
	    LORA_ERROR,
	    LORA_TIMEOUT,
	    LORA_CRC_ERROR,
	    LORA_NO_PACKET
} LoRa_Status; //define status of LoRa communication

LoRa_Status LoRa_Init(void); //initialization
LoRa_Status LoRa_Send(const uint8_t *data, //LoRa send (requires data, length, timeout in ms)
                      uint8_t length,
                      uint32_t timeout_ms);

LoRa_Status LoRa_Receive(uint8_t *data, //LoRa receive (requires data, capacity, length)
                         uint8_t capacity,
                         uint8_t *length);

void LoRa_StartReceive(void);
void LoRa_receiveConfigData(void);
uint8_t LoRa_ReadVersion(void);

#endif
