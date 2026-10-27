#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

char buffer[64];

void uint32_to_bin_str(uint32_t val, char *str) {
    int idx = 0;
    for (int i = 31; i >= 0; i--) {
        str[idx++] = (val & (1UL << i)) ? '1' : '0'; //Check each bit if it's 1 or 0
        // Optional: Add space every 8 bits for visual separation (e.g. 00000000 00000000 ...)
        if (i % 8 == 0 && i != 0) {
            str[idx++] = ' ';
        }
    }
    str[idx] = '\0';
}

void serial_result_display(UART_HandleTypeDef uart, double pressure, double altitude){
	HAL_UART_Transmit(&uart, (uint8_t*)"--------Result--------\r\n", 24, 100);

	snprintf(buffer, sizeof(buffer), "Raw pressure : %.2f Pa\r\n", pressure);
	HAL_UART_Transmit(&uart, (uint8_t*)buffer, sizeof(buffer), 100);

	memset(buffer, 0, sizeof(buffer)); //clear buffer
	snprintf(buffer, sizeof(buffer), "Altitude : %.2f m\r\n", altitude);
	HAL_UART_Transmit(&uart, (uint8_t*)buffer, sizeof(buffer), 100);

	// Array needs 32 digits + 3 spaces + 1 null terminator = 36 bytes (DEBUG ONLY!)
	//char bin_str[64];
	//uint32_to_bin_str((uint32_t)pressure, bin_str);

	//snprintf(buffer, sizeof(buffer), "Raw 32-bit bin: %s\r\n", bin_str);
	//HAL_UART_Transmit(&uart, (uint8_t*)buffer, strlen(buffer), 100);

	HAL_Delay(1000);
}
