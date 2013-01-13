# This program allows limited support to compile and run programs written for
# the Atmel ATTiny2313 to work like they were in a Hall Research KP2B keypad,
# only compiled for a native desktop environment with a Qt GUI instead of
# hardware buttons and LEDs.
# 
# Copyright 2012 David Fries.
# Released under the GPL v2.  See COPYING.

QT_FLAGS:=$(shell pkg-config --cflags QtGui) \
-I/usr/include/QtMobility -I/usr/include/QtMultimediaKit
QT_LIBS:=$(shell pkg-config --libs QtGui) -lQtMultimediaKit
CXXFLAGS=-g -Wall -std=c++11 -MMD -MP $(QT_FLAGS) -Iinclude -DF_CPU=8000000
LD_LIBS=$(QT_LIBS) -ldl

#AVR_SRC=../keypadContest/src/cpu_clock/cpu_clock.cc
#AVR_SRC=../keypadContest/src/input/input.cc
AVR_SRC=../keypadContest/src/nato_demo/hrt_example.cc

keypadalike: avr_target.o \
	avr_util.o avr_io.o \
	ATtiny.o ATtinyChip.o MicroMain.o moc_MicroMain.o \
	main.o SoftIO.o moc_SoftIO.o \
	SlotOwner.o moc_SlotOwner.o HallKeypad.o moc_HallKeypad.o \
	LEDWidget.o moc_LEDWidget.o \
	SquareAudio.o \
	Timer.o moc_Timer.o Timer0.o Timer1.o
	$(LINK.o) -o $@ $^

avr_target.o: $(AVR_SRC)
	$(COMPILE.cc) -o $@ $<

.PHONY: clean
clean:
	rm -f *.d *.o moc_*.cc moc_*.d moc_*.o keypadalike

moc_%.cc: %.h
	moc -o $@ $^

# Linking c++ not c code
LINK.o=$(CXX) $(LDFLAGS) $(LD_LIBS) $(TARGET_ARCH)

-include $(wildcard *.d)
