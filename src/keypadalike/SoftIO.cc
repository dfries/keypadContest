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
#include <QVBoxLayout>

SoftIO::SoftIO()
{
	QVBoxLayout *vlayout=new QVBoxLayout;
	QHBoxLayout *hlayout=new QHBoxLayout;
	for(int i=0; i<BUTTON_COUNT; ++i)
	{
		if(i==BUTTON_COUNT/2)
		{
			vlayout->addLayout(hlayout);
			hlayout=new QHBoxLayout;
		}
		Buttons[i]=new QPushButton(QString("%1").arg(i));
		hlayout->addWidget(Buttons[i]);
	}
	vlayout->addLayout(hlayout);
	setLayout(vlayout);
}
