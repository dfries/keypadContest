/*
This program allows limited support to compile and run programs written for
the Atmel ATTiny2313 to work like they were in a Hall Research KP2B keypad,
only compiled for a native desktop environment with a Qt GUI instead of
hardware buttons and LEDs.

Copyright 2012 David Fries.
Released under the GPL v2.  See COPYING.
*/

#include <QApplication>
#include "SoftIO.h"

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	SoftIO io;
	io.show();
	return app.exec();
}
