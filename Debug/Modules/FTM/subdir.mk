################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Modules/FTM/FTM.c 

OBJS += \
./Modules/FTM/FTM.o 

C_DEPS += \
./Modules/FTM/FTM.d 


# Each subdirectory must supply rules for building sources it contributes
Modules/FTM/%.o: ../Modules/FTM/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu11 -DCPU_MK64FN1M0VLL12 -DFRDM_K64F -DFREEDOM -DSERIAL_PORT_TYPE_UART=1 -DCPU_MK64FN1M0VLL12_cm4 -DSDK_DEBUGCONSOLE=0 -DCR_INTEGER_PRINTF -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__NEWLIB__ -I"C:\Users\User\git\Lab5\board" -I"C:\Users\User\git\Lab5\Library" -I"C:\Users\User\git\Lab5\Modules" -I"C:\Users\User\git\Lab5\source" -I"C:\Users\User\git\Lab5" -I"C:\Users\User\git\Lab5\drivers" -I"C:\Users\User\git\Lab5\device" -I"C:\Users\User\git\Lab5\CMSIS" -O0 -fno-common -g3 -Wall -c  -ffunction-sections  -fdata-sections  -ffreestanding  -fno-builtin -fmerge-constants -fmacro-prefix-map="../$(@D)/"=. -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -D__NEWLIB__ -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


