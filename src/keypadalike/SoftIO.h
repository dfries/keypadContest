/*
This program allows limited support to compile and run programs written for
the Atmel ATTiny2313 to work like they were in a Hall Research KP2B keypad,
only compiled for a native desktop environment with a Qt GUI instead of
hardware buttons and LEDs.

Copyright 2012 David Fries.
Released under the GPL v2.  See COPYING.
*/
#ifndef _SOFT_IO_H
#define _SOFT_IO_H

#include <QWidget>
class QPushButton;

/* Instead of the hardware buttons and LEDs, do software push buttons and
 * colored circles in software.
 */
class SoftIO : public QWidget
{
	Q_OBJECT
	enum {BUTTON_COUNT=10};
public:
	SoftIO();

private:
	QPushButton *Buttons[BUTTON_COUNT];
};

#endif //_SOFT_IO_H
