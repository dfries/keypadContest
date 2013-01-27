MCU=attiny2313
AVRDUDEMCU=attiny2313
FREQUENCY=8000000
# fuse settings: 8MHz internal oscillator, no divide-clock-by-8, and all other settings left default
FUSES=-U lfuse:w:0xe4:m -U hfuse:w:0xdf:m
CC=avr-gcc
CFLAGS=-g -Os -Wall -mcall-prologues -mmcu=$(MCU) -DF_CPU=$(FREQUENCY)
ELF2HEX=avr-objcopy
OBJDUMP=avr-objdump
AVRDUDE=avrdude
TARGET=recollection
SIZE=avr-size

# always build
.PHONY: all program

all: sizebefore $(TARGET).hex $(TARGET).lss sizeafter program

program : $(TARGET).hex
	sudo $(AVRDUDE) -p $(AVRDUDEMCU) -c usbtiny -U flash:w:$(TARGET).hex $(FUSES)

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
