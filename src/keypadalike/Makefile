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
CXXFLAGS=-g -Wall -std=c++11 -MMD -MP $(QT_FLAGS) -Iinclude -DF_CPU=8000000 \

#	-O2
LD_FLAGS=-L.
LD_LIBS=$(QT_LIBS) -ldl -lavr_target

# missing UART emulation
#AVR_SRC=../internetRadioControl/keypad-serial.c

# Any of the following lines can be given on make's command line to
# select the target.
AVR_SRC=../dfries_capture/capture.cc
#AVR_SRC=../input/input.c
#AVR_SRC=../cpu_clock/cpu_clock.cc
#AVR_SRC=../PatternRepeater/PatternRepeater.c
#AVR_SRC=../blinky/blinky.c
#AVR_SRC=../bongoHero/hero.c
#AVR_SRC=../input/input.c
#AVR_SRC=../nato_demo/hrt_example.c
#AVR_SRC=../recollection/recollection.c
#AVR_SRC=../rocketLaunch/rocket-launch.c
# runs for one turn, but it relies on the watchdog timer to reset for the
# next run which isn't functional in the emulation
#AVR_SRC=../super_wack_bros/super_wack_bros.c

all: libavr_target.so keypadalike

keypadalike: \
	avr_util.o avr_io.o \
	ATtiny.o ATtinyChip.o MicroMain.o moc_MicroMain.o \
	main.o SoftIO.o moc_SoftIO.o \
	SlotOwner.o moc_SlotOwner.o HallKeypad.o moc_HallKeypad.o \
	LEDWidget.o moc_LEDWidget.o \
	SquareAudio.o \
	Timer.o moc_Timer.o Timer0.o Timer1.o
	$(LINK.o) -o $@ $^

# force "-x c++" it to be compiled with C++ to get objects and overloading
avr_target.o: $(AVR_SRC)
	$(COMPILE.cc) -fPIC -x c++ -o $@ $<

libavr_target.so: avr_target.o
	$(LINK.cc) -shared $^ $(LOADLIBES) $(LDLIBS) -o $@

.PHONY: all clean
clean:
	rm -f *.d *.o moc_*.cc moc_*.d moc_*.o keypadalike libavr_target.so

moc_%.cc: %.h
	moc -o $@ $^

# Linking c++ not c code
LINK.o=$(CXX) $(LDFLAGS) $(LD_FLAGS) $(LD_LIBS) $(TARGET_ARCH)

-include $(wildcard *.d)
