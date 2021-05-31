################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../nfc/src/ad2s1210.cpp \
../nfc/src/arm.cpp \
../nfc/src/cr_cpp_config.cpp \
../nfc/src/cr_startup_lpc43xx.cpp \
../nfc/src/debug.cpp \
../nfc/src/dout.cpp \
../nfc/src/ds-18b20.cpp \
../nfc/src/eeprom.cpp \
../nfc/src/json_wp.cpp \
../nfc/src/lift.cpp \
../nfc/src/lwip_init.cpp \
../nfc/src/main.cpp \
../nfc/src/mem_check.cpp \
../nfc/src/mot_pap.cpp \
../nfc/src/net_commands.cpp \
../nfc/src/one-wire.cpp \
../nfc/src/parson.cpp \
../nfc/src/poncho_rdc.cpp \
../nfc/src/relay.cpp \
../nfc/src/settings.cpp \
../nfc/src/spi.cpp \
../nfc/src/tcp_server.cpp \
../nfc/src/temperature.cpp \
../nfc/src/temperature_ds18b20.cpp \
../nfc/src/tmr.cpp \
../nfc/src/wait.cpp 

C_SRCS += \
../nfc/src/crp.c \
../nfc/src/sysinit.c 

OBJS += \
./nfc/src/ad2s1210.o \
./nfc/src/arm.o \
./nfc/src/cr_cpp_config.o \
./nfc/src/cr_startup_lpc43xx.o \
./nfc/src/crp.o \
./nfc/src/debug.o \
./nfc/src/dout.o \
./nfc/src/ds-18b20.o \
./nfc/src/eeprom.o \
./nfc/src/json_wp.o \
./nfc/src/lift.o \
./nfc/src/lwip_init.o \
./nfc/src/main.o \
./nfc/src/mem_check.o \
./nfc/src/mot_pap.o \
./nfc/src/net_commands.o \
./nfc/src/one-wire.o \
./nfc/src/parson.o \
./nfc/src/poncho_rdc.o \
./nfc/src/relay.o \
./nfc/src/settings.o \
./nfc/src/spi.o \
./nfc/src/sysinit.o \
./nfc/src/tcp_server.o \
./nfc/src/temperature.o \
./nfc/src/temperature_ds18b20.o \
./nfc/src/tmr.o \
./nfc/src/wait.o 

CPP_DEPS += \
./nfc/src/ad2s1210.d \
./nfc/src/arm.d \
./nfc/src/cr_cpp_config.d \
./nfc/src/cr_startup_lpc43xx.d \
./nfc/src/debug.d \
./nfc/src/dout.d \
./nfc/src/ds-18b20.d \
./nfc/src/eeprom.d \
./nfc/src/json_wp.d \
./nfc/src/lift.d \
./nfc/src/lwip_init.d \
./nfc/src/main.d \
./nfc/src/mem_check.d \
./nfc/src/mot_pap.d \
./nfc/src/net_commands.d \
./nfc/src/one-wire.d \
./nfc/src/parson.d \
./nfc/src/poncho_rdc.d \
./nfc/src/relay.d \
./nfc/src/settings.d \
./nfc/src/spi.d \
./nfc/src/tcp_server.d \
./nfc/src/temperature.d \
./nfc/src/temperature_ds18b20.d \
./nfc/src/tmr.d \
./nfc/src/wait.d 

C_DEPS += \
./nfc/src/crp.d \
./nfc/src/sysinit.d 


# Each subdirectory must supply rules for building sources it contributes
nfc/src/%.o: ../nfc/src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C++ Compiler'
	arm-none-eabi-c++ -fpermissive -DDEBUG -D__CODE_RED -D__NEWLIB__ -DCORE_M4 -D__USE_LPCOPEN -DCPP_USE_HEAP -D__LPC43XX__ -I"/home/gustavo/FreeRTOS/CIAA_NXP_board/inc" -I"/home/gustavo/FreeRTOS/rtu_plusplus/lwip/inc/ipv4" -I"/home/gustavo/FreeRTOS/rtu_plusplus/lwip/inc" -I"/home/gustavo/FreeRTOS/rtu_plusplus/freertos/inc" -I"/home/gustavo/FreeRTOS/rtu_plusplus/nfc/inc" -I"/home/gustavo/MCUXpresso_11.2.0_4120/workspace/lpc_chip_43xx/inc" -I"/home/gustavo/MCUXpresso_11.2.0_4120/workspace/lpc_chip_43xx/inc/usbd" -O0 -fno-common -g3 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -fno-rtti -fno-exceptions -fsingle-precision-constant -fmerge-constants -fmacro-prefix-map="../$(@D)/"=. -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=softfp -mthumb -D__NEWLIB__ -fstack-usage -specs=nano.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

nfc/src/%.o: ../nfc/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu17 -DDEBUG -D__CODE_RED -D__NEWLIB__ -DCORE_M4 -D__USE_LPCOPEN -DCPP_USE_HEAP -D__LPC43XX__ -I"/home/gustavo/FreeRTOS/CIAA_NXP_board/inc" -I"/home/gustavo/FreeRTOS/rtu_plusplus/lwip/inc" -I"/home/gustavo/FreeRTOS/rtu_plusplus/lwip/inc/ipv4" -I"/home/gustavo/FreeRTOS/rtu_plusplus/freertos/inc" -I"/home/gustavo/FreeRTOS/rtu_plusplus/nfc/inc" -I"/home/gustavo/MCUXpresso_11.2.0_4120/workspace/lpc_chip_43xx/inc" -I"/home/gustavo/MCUXpresso_11.2.0_4120/workspace/lpc_chip_43xx/inc/usbd" -O0 -fno-common -g3 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -fsingle-precision-constant -fmerge-constants -fmacro-prefix-map="../$(@D)/"=. -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=softfp -mthumb -D__NEWLIB__ -fstack-usage -specs=nano.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


