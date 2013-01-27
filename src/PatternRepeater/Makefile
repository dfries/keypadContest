MCU=attiny2313
AVRDUDEMCU=attiny2313
CC=avr-gcc
CFLAGS=-g -Os -Wall -mcall-prologues -mmcu=$(MCU) -std=gnu99
ELF2HEX=avr-objcopy
OBJDUMP=avr-objdump
AVRDUDE=avrdude
TARGET=PatternRepeater
SIZE=avr-size

CFLAGS:=$(CFLAGS) -DF_CPU=8000000

# always build
.PHONY: all program

all: sizebefore $(TARGET).hex $(TARGET).lss sizeafter program

program : $(TARGET).hex
	$(AVRDUDE) -p $(AVRDUDEMCU) -c usbtiny -U flash:w:$(TARGET).hex

%.elf : %.o
	$(CC) $(CFLAGS) $< -o $@

%.hex : %.elf
	$(ELF2HEX) -R .eeprom -O ihex $< $@

# Prints the previous and current executable sizes, useful to keep track of
# the program growth.
# text: program code and flash allocate data
# data: constants used to initialize static variables, takes flash and ram
# bss: static variables not assigned constants, ram only
# dec: total in decimal
# hex: total in hex
sizebefore:
	@cp $(TARGET).elf $(TARGET).elf.old 2> /dev/null || true

sizeafter: $(TARGET).elf
	@if test -f $(TARGET).elf; then \
	if test -f $(TARGET).elf.old; then \
		echo Size before:; $(SIZE) $(TARGET).elf.old; fi; \
	if cmp --quiet $(TARGET).elf.old $(TARGET).elf; then \
		echo Size after: file is identical; else \
		echo Size after:; $(SIZE) $(TARGET).elf; \
	fi 2>/dev/null; fi

# extended assembly listing file
%.lss: %.elf
	$(OBJDUMP) -h -S -z $< > $@

clean :
	rm -f *.hex *.elf *.elf.old *.lss *.o
