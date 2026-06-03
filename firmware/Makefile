# ATtiny85 USB HID - Makefile
MCU = attiny85
F_CPU = 16500000
CC = avr-gcc
OBJCOPY = avr-objcopy
AVRDUDE = avrdude
PROGRAMMER = usbasp

CFLAGS = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Os -Wall -std=c99
CFLAGS += -Iusbdrv

TARGET = main

SOURCES = main.c usbdrv/usbdrv.c usbdrv/usbdrvasm.S

all: $(TARGET).hex

$(TARGET).elf: $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $^

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

flash: $(TARGET).hex
	$(AVRDUDE) -c $(PROGRAMMER) -p $(MCU) -U flash:w:$(TARGET).hex

fuses:
	$(AVRDUDE) -c $(PROGRAMMER) -p $(MCU) -U lfuse:w:0xE1:m -U hfuse:w:0xDD:m

clean:
	del $(TARGET).elf $(TARGET).hex