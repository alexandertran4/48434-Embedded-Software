################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include startup/subdir.mk
-include source/subdir.mk
-include drivers/subdir.mk
-include device/subdir.mk
-include board/subdir.mk
-include VRR/subdir.mk
-include Modules/UART/subdir.mk
-include Modules/Packet/subdir.mk
-include Modules/PIT/subdir.mk
-include Modules/Median/subdir.mk
-include Modules/LEDs/subdir.mk
-include Modules/Flash/subdir.mk
-include Modules/FTM/subdir.mk
-include Modules/FIFO/subdir.mk
-include Modules/Critical/subdir.mk
-include Modules/ADC/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: Project.axf

# Tool invocations
Project.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -L"C:\Users\User\git\Project\Library" -Xlinker -Map="Project.map" -Xlinker --gc-sections -Xlinker -print-memory-usage -Xlinker --sort-section=alignment -Xlinker --cref -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -T Project_Debug.ld -o "Project.axf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) Project.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "Project.axf"; # arm-none-eabi-objcopy -v -O binary "Project.axf" "Project.bin" ; # checksum -p MK64FN1M0xxx12 -d "Project.bin";
	-@echo ' '

.PHONY: all clean dependents post-build

-include ../makefile.targets
