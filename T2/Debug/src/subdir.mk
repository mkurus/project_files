################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/MMA8652.c \
../src/ProcessTask.c \
../src/adc.c \
../src/buzzer_task.c \
../src/can.c \
../src/cr_startup_lpc175x_6x.c \
../src/crp.c \
../src/din_task.c \
../src/dout_task.c \
../src/event.c \
../src/gSensor.c \
../src/gps.c \
../src/gsm.c \
../src/gsm_task.c \
../src/hx711.c \
../src/i2c.c \
../src/ign_task.c \
../src/io_ctrl.c \
../src/json_parser.c \
../src/le50.c \
../src/lora.c \
../src/lora_task.c \
../src/main.c \
../src/messages.c \
../src/msg_parser.c \
../src/offline.c \
../src/offline_task.c \
../src/onewire.c \
../src/rf_task.c \
../src/rfid.c \
../src/scale_task.c \
../src/settings.c \
../src/spi.c \
../src/sst25.c \
../src/status.c \
../src/sysinit.c \
../src/tablet_app.c \
../src/timer.c \
../src/timer_task.c \
../src/trace.c \
../src/utils.c 

OBJS += \
./src/MMA8652.o \
./src/ProcessTask.o \
./src/adc.o \
./src/buzzer_task.o \
./src/can.o \
./src/cr_startup_lpc175x_6x.o \
./src/crp.o \
./src/din_task.o \
./src/dout_task.o \
./src/event.o \
./src/gSensor.o \
./src/gps.o \
./src/gsm.o \
./src/gsm_task.o \
./src/hx711.o \
./src/i2c.o \
./src/ign_task.o \
./src/io_ctrl.o \
./src/json_parser.o \
./src/le50.o \
./src/lora.o \
./src/lora_task.o \
./src/main.o \
./src/messages.o \
./src/msg_parser.o \
./src/offline.o \
./src/offline_task.o \
./src/onewire.o \
./src/rf_task.o \
./src/rfid.o \
./src/scale_task.o \
./src/settings.o \
./src/spi.o \
./src/sst25.o \
./src/status.o \
./src/sysinit.o \
./src/tablet_app.o \
./src/timer.o \
./src/timer_task.o \
./src/trace.o \
./src/utils.o 

C_DEPS += \
./src/MMA8652.d \
./src/ProcessTask.d \
./src/adc.d \
./src/buzzer_task.d \
./src/can.d \
./src/cr_startup_lpc175x_6x.d \
./src/crp.d \
./src/din_task.d \
./src/dout_task.d \
./src/event.d \
./src/gSensor.d \
./src/gps.d \
./src/gsm.d \
./src/gsm_task.d \
./src/hx711.d \
./src/i2c.d \
./src/ign_task.d \
./src/io_ctrl.d \
./src/json_parser.d \
./src/le50.d \
./src/lora.d \
./src/lora_task.d \
./src/main.d \
./src/messages.d \
./src/msg_parser.d \
./src/offline.d \
./src/offline_task.d \
./src/onewire.d \
./src/rf_task.d \
./src/rfid.d \
./src/scale_task.d \
./src/settings.d \
./src/spi.d \
./src/sst25.d \
./src/status.d \
./src/sysinit.d \
./src/tablet_app.d \
./src/timer.d \
./src/timer_task.d \
./src/trace.d \
./src/utils.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -DDEBUG -D__CODE_RED -DCORE_M3 -D__USE_LPCOPEN -D__LPC17XX__ -D__NEWLIB__ -I"C:\Users\mkurus\Desktop\backup\T2\R0071\lpc_board_nxp_lpcxpresso_1769\inc" -I"C:\Users\mkurus\Desktop\backup\T2\R0071\lpc_chip_175x_6x\inc" -O1 -g1 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -flto -ffat-lto-objects -mcpu=cortex-m3 -mthumb -D__NEWLIB__ -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


