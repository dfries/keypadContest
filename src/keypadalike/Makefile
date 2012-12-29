# This program allows limited support to compile and run programs written for
# the Atmel ATTiny2313 to work like they were in a Hall Research KP2B keypad,
# only compiled for a native desktop environment with a Qt GUI instead of
# hardware buttons and LEDs.
# 
# Copyright 2012 David Fries.
# Released under the GPL v2.  See COPYING.

QT_FLAGS:=$(shell pkg-config --cflags QtGui)
QT_LIBS:=$(shell pkg-config --libs QtGui)
CXXFLAGS=-g -Wall -std=c++11 -MMD -MP $(QT_FLAGS) -Iinclude -DF_CPU=8000000
LD_LIBS=$(QT_LIBS)

AVR_OBJ=../keypadContest/src/cpu_clock/cpu_clock.o
keypadalike: $(AVR_OBJ) \
	avr_util.o avr_io.o \
	ATtiny.o ATtinyChip.o MicroMain.o moc_MicroMain.o \
	main.o SoftIO.o moc_SoftIO.o \
	SlotOwner.o moc_SlotOwner.o HallKeypad.o moc_HallKeypad.o
	$(LINK.o) -o $@ $^

.PHONY: clean
clean:
	rm -f *.d *.o moc_*.cc moc_*.d moc_*.o keypadalike

moc_%.cc: %.h
	moc -o $@ $^

# Linking c++ not c code
LINK.o=$(CXX) $(LDFLAGS) $(LD_LIBS) $(TARGET_ARCH)

-include $(wildcard *.d)
