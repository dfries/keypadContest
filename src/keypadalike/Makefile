# This program allows limited support to compile and run programs written for
# the Atmel ATTiny2313 to work like they were in a Hall Research KP2B keypad,
# only compiled for a native desktop environment with a Qt GUI instead of
# hardware buttons and LEDs.
# 
# Copyright 2012 David Fries.
# Released under the GPL v2.  See COPYING.

QT_FLAGS:=$(shell pkg-config --cflags QtGui)
QT_LIBS:=$(shell pkg-config --libs QtGui)
CXXFLAGS=-g -Wall -MMD -MP $(QT_FLAGS)
LD_LIBS=$(QT_LIBS)

# Linking c++ not c code
LINK.o=$(CXX) $(LDFLAGS) $(LD_LIBS) $(TARGET_ARCH)

keypadalike: main.o SoftIO.o moc_SoftIO.o
	$(LINK.o) -o $@ $^

moc_%.cc: %.h
	moc -o $@ $^

.PHONY: clean
clean:
	rm -f *.d *.o moc_*.cc moc_*.d moc_*.o keypadalike

-include $(wildcard *.d)
