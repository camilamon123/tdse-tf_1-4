################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/drivers/ldr.c \
../Core/Src/drivers/ssd1306.c 

OBJS += \
./Core/Src/drivers/ldr.o \
./Core/Src/drivers/ssd1306.o 

C_DEPS += \
./Core/Src/drivers/ldr.d \
./Core/Src/drivers/ssd1306.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/drivers/%.o Core/Src/drivers/%.su Core/Src/drivers/%.cyclo: ../Core/Src/drivers/%.c Core/Src/drivers/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-drivers

clean-Core-2f-Src-2f-drivers:
	-$(RM) ./Core/Src/drivers/ldr.cyclo ./Core/Src/drivers/ldr.d ./Core/Src/drivers/ldr.o ./Core/Src/drivers/ldr.su ./Core/Src/drivers/ssd1306.cyclo ./Core/Src/drivers/ssd1306.d ./Core/Src/drivers/ssd1306.o ./Core/Src/drivers/ssd1306.su

.PHONY: clean-Core-2f-Src-2f-drivers

