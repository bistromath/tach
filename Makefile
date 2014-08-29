# setup
# DO NOT USE -Os UNLESS YOU ALSO PREVENT STRUCT PACKING
COMPILE_OPTS = \
	-mthumb \
	-mcpu=cortex-m3 \
	-Wall \
	-g \
	-O2 \
	-msoft-float \
	-ffunction-sections \
	-D__thumb2__ \
	-DSTM32F10X_MD \
	-DHSE_VALUE=16000000 \
	-DSYSCLK_FREQ_72MHz \
	-mthumb-interwork \
	-mfix-cortex-m3-ldrd \
	-mabi=aapcs \
	-fno-optimize-sibling-calls \
	-DDEBUG

INCLUDE_DIRS = \
	-I inc \
	-I newlib/STM32F10x_StdPeriph_Driver/inc \
	-I newlib/CMSIS/CM3/DeviceSupport/ST/STM32F10x \
	-I newlib/CMSIS/CM3/CoreSupport

LIBRARY_DIRS = \
	-L newlib \
	-L newlib/CMSIS/CM3/DeviceSupport/ST/STM32F10x \
	-L usblib

#LIBRARY_DIRS = -L lib -L /home/bistromath/Desktop/arm-2008q1/arm-none-eabi/lib

CC = arm-none-eabi-gcc
CFLAGS = $(COMPILE_OPTS) $(INCLUDE_DIRS) -include inc/stm32f10x_conf.h

CXX = arm-none-eabi-g++
CXXFLAGS = $(COMPILE_OPTS) $(INCLUDE_DIRS) -fno-exceptions -fcheck-new -std=c++0x

AS = arm-none-eabi-gcc
ASFLAGS = $(COMPILE_OPTS) -c

LD = arm-none-eabi-g++
LDFLAGS = $(COMPILE_OPTS) -Wl,--gc-sections,-Map=$@.map,-cref,-Reset_Handler $(INCLUDE_DIRS) $(LIBRARY_DIRS) -T STM32F103_ROM.ld -lstdc++

AR = arm-none-eabi-ar
ARFLAGS = cr

MAIN_OUT = main
MAIN_OUT_ELF = $(MAIN_OUT).elf
MAIN_OUT_BIN = $(MAIN_OUT).bin
MAIN_OUT_BIN_STATE = $(MAIN_OUT)-state.bin
MAIN_OUT_BIN_CAL = $(MAIN_OUT)-cal.bin
MAIN_OUT_BIN_ALL = $(MAIN_OUT)-all.bin

LIBCORE_OUT = newlib/CMSIS/CM3/libcore.a
LIBSTM32_OUT = newlib/libstm32.a

# all
all: $(MAIN_OUT_ELF)

OBJS =			 src/startup.o \
				 src/stepper.o \
				 src/system_stm32f10x.o \
				 src/stm32f10x_vectors.o \
				 src/main.o \
				 src/stm32f10x_it.o \
				 src/i2c_device.o \
				 src/pg9616.o \
				 src/systick.o \
				 src/tick_callback.o \
				 src/syscalls.o \
				 src/newdel.o \
				 src/tach.o \
				 src/adc.o \
				 $(LIBCORE_OUT) \
				 $(LIBSTM32_OUT)

$(MAIN_OUT_ELF): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) --output $@

$(MAIN_OUT_BIN): $(MAIN_OUT_ELF)
	$(OBJCP) $(OBJCPFLAGS) $< $@

$(MAIN_OUT_BIN_STATE): $(MAIN_OUT_ELF)
	$(OBJCP) $(STATEOBJCPFLAGS) $< $@

$(MAIN_OUT_BIN_CAL): $(MAIN_OUT_ELF)
	$(OBJCP) $(CALOBJCPFLAGS) $< $@
	
$(MAIN_OUT_BIN_ALL): $(MAIN_OUT_ELF)
	$(OBJCP) $(ALLOBJCPFLAGS) $< $@

# flash

LIBCORE_OBJS = \
 newlib/CMSIS/CM3/CoreSupport/core_cm3.o \

$(LIBCORE_OUT): $(LIBCORE_OBJS)
	$(AR) $(ARFLAGS) $@ $(LIBCORE_OBJS)

$(LIBCORE_OBJS): inc/stm32f10x_conf.h

LIBSTM32_OBJS = \
 newlib/STM32F10x_StdPeriph_Driver/src/stm32f10x_adc.o \
 newlib/STM32F10x_StdPeriph_Driver/src/stm32f10x_bkp.o \
 newlib/STM32F10x_StdPeriph_Driver/src/stm32f10x_can.o \
 newlib/STM32F10x_StdPeriph_Driver/src/stm32f10x_crc.o \
 newlib/STM32F10x_StdPeriph_Driver/src/stm32f10x_dac.o \
 newlib/STM32F10x_StdPeriph_Driver/src/stm32f10x_dbgmcu.o \
 newlib/STM32F10x_StdPeriph_Driver/src/stm32f10x_dma.o \
 newlib/STM32F10x_StdPeriph_Driver/src/stm32f10x_exti.o \
 newlib/STM32F10x_StdPeriph_Driver/src/stm32f10x_flash.o \
 newlib/STM32F10x_StdPeriph_Driver/src/stm32f10x_fsmc.o \
 newlib/STM32F10x_StdPeriph_Driver/src/stm32f10x_gpio.o \
 newlib/STM32F10x_StdPeriph_Driver/src/stm32f10x_i2c.o \
 newlib/STM32F10x_StdPeriph_Driver/src/stm32f10x_iwdg.o \
 newlib/STM32F10x_StdPeriph_Driver/src/stm32f10x_pwr.o \
 newlib/STM32F10x_StdPeriph_Driver/src/stm32f10x_rcc.o \
 newlib/STM32F10x_StdPeriph_Driver/src/stm32f10x_rtc.o \
 newlib/STM32F10x_StdPeriph_Driver/src/stm32f10x_sdio.o \
 newlib/STM32F10x_StdPeriph_Driver/src/stm32f10x_spi.o \
 newlib/STM32F10x_StdPeriph_Driver/src/stm32f10x_tim.o \
 newlib/STM32F10x_StdPeriph_Driver/src/stm32f10x_usart.o \
 newlib/STM32F10x_StdPeriph_Driver/src/stm32f10x_wwdg.o	\
 newlib/STM32F10x_StdPeriph_Driver/src/misc.o \
 $(LIBCORE_OUT)

$(LIBSTM32_OUT): $(LIBSTM32_OBJS) $(LIBCORE_OUT)
	$(AR) $(ARFLAGS) $@ $(LIBSTM32_OBJS)
 	
$(LIBSTM32_OBJS): inc/stm32f10x_conf.h

clean:
	-rm src/*.o $(LIBSTM32_OUT) $(LIBCORE_OUT) $(MAIN_OUT_ELF) $(MAIN_OUT_ELF).map
