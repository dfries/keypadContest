/*
This program allows limited support to compile and run programs written for
the Atmel ATTiny2313 to work like they were in a Hall Research KP2B keypad,
only compiled for a native desktop environment with a Qt GUI instead of
hardware buttons and LEDs.

Copyright 2012 David Fries.
Released under the GPL v2.  See COPYING.
*/

#include "SoftIO.h"
#include <QPushButton>
#include <QHBoxLayout>

SoftIO::SoftIO()
{
	QHBoxLayout *layout=new QHBoxLayout;
	for(int i=0; i<BUTTON_COUNT; ++i)
	{
		Buttons[i]=new QPushButton(QString("%1").arg(i));
		layout->addWidget(Buttons[i]);
	}
	setLayout(layout);
}
