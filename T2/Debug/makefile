################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
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
all: T2.axf

# Tool invocations
T2.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -L"C:\Users\mkurus\Desktop\backup\T2\R0071\lpc_board_nxp_lpcxpresso_1769\Debug" -L"C:\Users\mkurus\Desktop\backup\T2\R0071\lpc_chip_175x_6x\Debug" -Xlinker -Map="T2.map" -Xlinker --gc-sections -flto -O1 -mcpu=cortex-m3 -mthumb -u _printf_float -T "Trio_Custom.ld" -o "T2.axf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) T2.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size T2.axf ; arm-none-eabi-objdump -S T2.axf >T2.lss ; arm-none-eabi-objcopy -O ihex T2.axf T2.hex ; arm-none-eabi-objcopy -O binary T2.axf T2.bin
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY: post-build

-include ../makefile.targets
