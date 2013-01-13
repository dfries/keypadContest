MCU=attiny2313
AVRDUDEMCU=attiny2313
CC=avr-gcc
CFLAGS=-g -Os -Wall -mcall-prologues -mmcu=$(MCU)
OBJ2HEX=avr-objcopy
AVRDUDE=avrdude
TARGET=rocket-launch

program : $(TARGET).hex
	$(AVRDUDE) -p $(AVRDUDEMCU) -c usbtiny -U flash:w:$(TARGET).hex

all : $(TARGET).hex

%.obj : %.o
	$(CC) $(CFLAGS) $< -o $@

%.hex : %.obj
	$(OBJ2HEX) -R .eeprom -O ihex $< $@

clean :
	rm -f *.hex *.obj *.o
