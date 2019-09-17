################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/MMA8652.c \
../src/ProcessTask.c \
../src/adc.c \
../src/can.c \
../src/cr_startup_lpc175x_6x.c \
../src/crp.c \
../src/event.c \
../src/gSensor.c \
../src/gps.c \
../src/gsm.c \
../src/i2c.c \
../src/io_ctrl.c \
../src/json_parser.c \
../src/le50.c \
../src/main.c \
../src/messages.c \
../src/msg_parser.c \
../src/offline.c \
../src/onewire.c \
../src/rf_task.c \
../src/rfid.c \
../src/settings.c \
../src/spi.c \
../src/sst25.c \
../src/status.c \
../src/sysinit.c \
../src/tablet_app.c \
../src/timer.c \
../src/trace.c \
../src/utils.c 

OBJS += \
./src/MMA8652.o \
./src/ProcessTask.o \
./src/adc.o \
./src/can.o \
./src/cr_startup_lpc175x_6x.o \
./src/crp.o \
./src/event.o \
./src/gSensor.o \
./src/gps.o \
./src/gsm.o \
./src/i2c.o \
./src/io_ctrl.o \
./src/json_parser.o \
./src/le50.o \
./src/main.o \
./src/messages.o \
./src/msg_parser.o \
./src/offline.o \
./src/onewire.o \
./src/rf_task.o \
./src/rfid.o \
./src/settings.o \
./src/spi.o \
./src/sst25.o \
./src/status.o \
./src/sysinit.o \
./src/tablet_app.o \
./src/timer.o \
./src/trace.o \
./src/utils.o 

C_DEPS += \
./src/MMA8652.d \
./src/ProcessTask.d \
./src/adc.d \
./src/can.d \
./src/cr_startup_lpc175x_6x.d \
./src/crp.d \
./src/event.d \
./src/gSensor.d \
./src/gps.d \
./src/gsm.d \
./src/i2c.d \
./src/io_ctrl.d \
./src/json_parser.d \
./src/le50.d \
./src/main.d \
./src/messages.d \
./src/msg_parser.d \
./src/offline.d \
./src/onewire.d \
./src/rf_task.d \
./src/rfid.d \
./src/settings.d \
./src/spi.d \
./src/sst25.d \
./src/status.d \
./src/sysinit.d \
./src/tablet_app.d \
./src/timer.d \
./src/trace.d \
./src/utils.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -DNDEBUG -D__CODE_RED -DCORE_M3 -D__USE_LPCOPEN -D__LPC17XX__ -D__NEWLIB__ -I"C:\Users\mkurus\Desktop\backup\T2\R0037\lpc_board_nxp_lpcxpresso_1769\inc" -I"C:\Users\mkurus\Desktop\backup\T2\R0037\lpc_chip_175x_6x\inc" -Os -g -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -mcpu=cortex-m3 -mthumb -D__NEWLIB__ -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


