/*
This program allows limited support to compile and run programs written for
the Atmel ATTiny2313 to work like they were in a Hall Research KP2B keypad,
only compiled for a native desktop environment with a Qt GUI instead of
hardware buttons and LEDs.

    Copyright (C) Copyright 2012 David Fries.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "SoftIO.h"
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "LEDWidget.h"
#include "SlotOwner.h"

SoftIO::SoftIO() :
	LEDState(0)
{
	QVBoxLayout *vlayout=new QVBoxLayout;
	QHBoxLayout *hled=new QHBoxLayout, *hbutton=new QHBoxLayout;
	for(int i=0; i<BUTTON_COUNT; ++i)
	{
		if(i==BUTTON_COUNT/2)
		{
			vlayout->addLayout(hled);
			vlayout->addLayout(hbutton);
			hled=new QHBoxLayout;
			hbutton=new QHBoxLayout;
		}
		LEDs[i]=new LEDWidget;
		hled->addWidget(LEDs[i]);
		Buttons[i]=new QPushButton(QString("%1").arg(i));
		hbutton->addWidget(Buttons[i]);
		// Bounce the signal through a unique object to be able to
		// identify which object sent it when it gets to the local slot.
		SlotOwner *owner=new SlotOwner(Buttons[i], this);
		connect(Buttons[i], SIGNAL(clicked(bool)),
			owner, SLOT(SlotA(bool)));
		connect(owner, SIGNAL(SignalA(bool, QObject*)),
			SLOT(Clicked(bool, QObject*)));
	}
	vlayout->addLayout(hled);
	vlayout->addLayout(hbutton);
	setLayout(vlayout);
}

void SoftIO::SetLEDs(uint16_t led)
{
	if(led == LEDState)
		return;

	for(int i=0; i<BUTTON_COUNT; ++i)
	{
		int b=1<<i;
		// update if required
		if((led ^ LEDState) & b)
			LEDs[i]->SetOn(led & b);
	}
	LEDState=led;
}

void SoftIO::Clicked(bool checked, QObject *sender)
{
	QAbstractButton *button=dynamic_cast<QAbstractButton*>(button);
	if(!button)
		return;
	for(int i=0; i<BUTTON_COUNT; ++i)
	{
		if(button==Buttons[i])
		{
			ButtonState &= ~(1<<i);
			ButtonState |= checked<<i;
			SetButtons(ButtonState);
			break;
		}
	}
}
