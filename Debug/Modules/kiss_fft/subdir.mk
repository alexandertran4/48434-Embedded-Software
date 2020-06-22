################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Modules/kiss_fft/kiss_fft.c \
../Modules/kiss_fft/kiss_fftr.c 

OBJS += \
./Modules/kiss_fft/kiss_fft.o \
./Modules/kiss_fft/kiss_fftr.o 

C_DEPS += \
./Modules/kiss_fft/kiss_fft.d \
./Modules/kiss_fft/kiss_fftr.d 


# Each subdirectory must supply rules for building sources it contributes
Modules/kiss_fft/%.o: ../Modules/kiss_fft/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu11 -DCPU_MK64FN1M0VLL12 -DFRDM_K64F -DFREEDOM -DSERIAL_PORT_TYPE_UART=1 -DCPU_MK64FN1M0VLL12_cm4 -DSDK_DEBUGCONSOLE=0 -DCR_INTEGER_PRINTF -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__NEWLIB__ -I"C:\Users\User\git\Project\board" -I"C:\Users\User\git\Project\Library" -I"C:\Users\User\git\Project\Modules" -I"C:\Users\User\git\Project\source" -I"C:\Users\User\git\Project" -I"C:\Users\User\git\Project\drivers" -I"C:\Users\User\git\Project\device" -I"C:\Users\User\git\Project\CMSIS" -O0 -fno-common -g3 -Wall -c  -ffunction-sections  -fdata-sections  -ffreestanding  -fno-builtin -fmerge-constants -fmacro-prefix-map="../$(@D)/"=. -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -D__NEWLIB__ -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


