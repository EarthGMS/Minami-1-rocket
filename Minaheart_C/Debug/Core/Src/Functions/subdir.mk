################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/Functions/SD_card.c \
../Core/Src/Functions/pressure_sensor.c \
../Core/Src/Functions/telemetry_debug.c \
../Core/Src/Functions/timer.c 

OBJS += \
./Core/Src/Functions/SD_card.o \
./Core/Src/Functions/pressure_sensor.o \
./Core/Src/Functions/telemetry_debug.o \
./Core/Src/Functions/timer.o 

C_DEPS += \
./Core/Src/Functions/SD_card.d \
./Core/Src/Functions/pressure_sensor.d \
./Core/Src/Functions/telemetry_debug.d \
./Core/Src/Functions/timer.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/Functions/%.o Core/Src/Functions/%.su Core/Src/Functions/%.cyclo: ../Core/Src/Functions/%.c Core/Src/Functions/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F411xE -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../FATFS/Target -I../FATFS/App -I../Middlewares/Third_Party/FatFs/src -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-Functions

clean-Core-2f-Src-2f-Functions:
	-$(RM) ./Core/Src/Functions/SD_card.cyclo ./Core/Src/Functions/SD_card.d ./Core/Src/Functions/SD_card.o ./Core/Src/Functions/SD_card.su ./Core/Src/Functions/pressure_sensor.cyclo ./Core/Src/Functions/pressure_sensor.d ./Core/Src/Functions/pressure_sensor.o ./Core/Src/Functions/pressure_sensor.su ./Core/Src/Functions/telemetry_debug.cyclo ./Core/Src/Functions/telemetry_debug.d ./Core/Src/Functions/telemetry_debug.o ./Core/Src/Functions/telemetry_debug.su ./Core/Src/Functions/timer.cyclo ./Core/Src/Functions/timer.d ./Core/Src/Functions/timer.o ./Core/Src/Functions/timer.su

.PHONY: clean-Core-2f-Src-2f-Functions

