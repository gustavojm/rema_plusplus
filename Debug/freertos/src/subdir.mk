################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../freertos/src/croutine.c \
../freertos/src/event_groups.c \
../freertos/src/heap_4.c \
../freertos/src/list.c \
../freertos/src/port.c \
../freertos/src/queue.c \
../freertos/src/stream_buffer.c \
../freertos/src/tasks.c \
../freertos/src/timers.c 

OBJS += \
./freertos/src/croutine.o \
./freertos/src/event_groups.o \
./freertos/src/heap_4.o \
./freertos/src/list.o \
./freertos/src/port.o \
./freertos/src/queue.o \
./freertos/src/stream_buffer.o \
./freertos/src/tasks.o \
./freertos/src/timers.o 

C_DEPS += \
./freertos/src/croutine.d \
./freertos/src/event_groups.d \
./freertos/src/heap_4.d \
./freertos/src/list.d \
./freertos/src/port.d \
./freertos/src/queue.d \
./freertos/src/stream_buffer.d \
./freertos/src/tasks.d \
./freertos/src/timers.d 


# Each subdirectory must supply rules for building sources it contributes
freertos/src/%.o: ../freertos/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu17 -DDEBUG -D__CODE_RED -D__NEWLIB__ -DCORE_M4 -D__USE_LPCOPEN -DCPP_USE_HEAP -D__LPC43XX__ -I"/home/gustavo/FreeRTOS/CIAA_NXP_board/inc" -I"/home/gustavo/FreeRTOS/rtu_plusplus/lwip/inc" -I"/home/gustavo/FreeRTOS/rtu_plusplus/lwip/inc/ipv4" -I"/home/gustavo/FreeRTOS/rtu_plusplus/freertos/inc" -I"/home/gustavo/FreeRTOS/rtu_plusplus/nfc/inc" -I"/home/gustavo/MCUXpresso_11.2.0_4120/workspace/lpc_chip_43xx/inc" -I"/home/gustavo/MCUXpresso_11.2.0_4120/workspace/lpc_chip_43xx/inc/usbd" -O0 -fno-common -g3 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -fsingle-precision-constant -fmerge-constants -fmacro-prefix-map="../$(@D)/"=. -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=softfp -mthumb -D__NEWLIB__ -fstack-usage -specs=nano.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


