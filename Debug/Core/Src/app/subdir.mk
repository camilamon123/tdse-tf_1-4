################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/app/app.c \
../Core/Src/app/bt_link.c \
../Core/Src/app/button_input.c \
../Core/Src/app/buzzer.c \
../Core/Src/app/led_ui.c \
../Core/Src/app/light_sensor.c \
../Core/Src/app/morse_decoder.c \
../Core/Src/app/nv_store.c \
../Core/Src/app/oled_ui.c \
../Core/Src/app/setup_btn.c 

OBJS += \
./Core/Src/app/app.o \
./Core/Src/app/bt_link.o \
./Core/Src/app/button_input.o \
./Core/Src/app/buzzer.o \
./Core/Src/app/led_ui.o \
./Core/Src/app/light_sensor.o \
./Core/Src/app/morse_decoder.o \
./Core/Src/app/nv_store.o \
./Core/Src/app/oled_ui.o \
./Core/Src/app/setup_btn.o 

C_DEPS += \
./Core/Src/app/app.d \
./Core/Src/app/bt_link.d \
./Core/Src/app/button_input.d \
./Core/Src/app/buzzer.d \
./Core/Src/app/led_ui.d \
./Core/Src/app/light_sensor.d \
./Core/Src/app/morse_decoder.d \
./Core/Src/app/nv_store.d \
./Core/Src/app/oled_ui.d \
./Core/Src/app/setup_btn.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/app/%.o Core/Src/app/%.su Core/Src/app/%.cyclo: ../Core/Src/app/%.c Core/Src/app/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-app

clean-Core-2f-Src-2f-app:
	-$(RM) ./Core/Src/app/app.cyclo ./Core/Src/app/app.d ./Core/Src/app/app.o ./Core/Src/app/app.su ./Core/Src/app/bt_link.cyclo ./Core/Src/app/bt_link.d ./Core/Src/app/bt_link.o ./Core/Src/app/bt_link.su ./Core/Src/app/button_input.cyclo ./Core/Src/app/button_input.d ./Core/Src/app/button_input.o ./Core/Src/app/button_input.su ./Core/Src/app/buzzer.cyclo ./Core/Src/app/buzzer.d ./Core/Src/app/buzzer.o ./Core/Src/app/buzzer.su ./Core/Src/app/led_ui.cyclo ./Core/Src/app/led_ui.d ./Core/Src/app/led_ui.o ./Core/Src/app/led_ui.su ./Core/Src/app/light_sensor.cyclo ./Core/Src/app/light_sensor.d ./Core/Src/app/light_sensor.o ./Core/Src/app/light_sensor.su ./Core/Src/app/morse_decoder.cyclo ./Core/Src/app/morse_decoder.d ./Core/Src/app/morse_decoder.o ./Core/Src/app/morse_decoder.su ./Core/Src/app/nv_store.cyclo ./Core/Src/app/nv_store.d ./Core/Src/app/nv_store.o ./Core/Src/app/nv_store.su ./Core/Src/app/oled_ui.cyclo ./Core/Src/app/oled_ui.d ./Core/Src/app/oled_ui.o ./Core/Src/app/oled_ui.su ./Core/Src/app/setup_btn.cyclo ./Core/Src/app/setup_btn.d ./Core/Src/app/setup_btn.o ./Core/Src/app/setup_btn.su

.PHONY: clean-Core-2f-Src-2f-app

