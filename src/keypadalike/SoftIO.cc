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
#include <QCheckBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QKeyEvent>
#include "LEDWidget.h"
#include "SlotOwner.h"
#define XK_MISCELLANY
#include <X11/keysymdef.h>

#include <stdio.h>

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
		// 1 based labels
		Buttons[i]=new QCheckBox(QString("%1").arg(i+1));
		hbutton->addWidget(Buttons[i]);
		// Bounce the signal through a unique object to be able to
		// identify which object sent it when it gets to the local slot.
		SlotOwner *owner=new SlotOwner(Buttons[i], this);
		connect(Buttons[i], SIGNAL(stateChanged(int)),
			owner, SLOT(SlotA(int)));
		connect(owner, SIGNAL(SignalA(int, QObject*)),
			SLOT(Clicked(int, QObject*)));
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

void SoftIO::Clicked(int state, QObject *sender)
{
	QAbstractButton *button=dynamic_cast<QAbstractButton*>(sender);
	if(!button)
		return;
	for(int i=0; i<BUTTON_COUNT; ++i)
	{
		if(button==Buttons[i])
		{
			ButtonState &= ~(1<<i);
			ButtonState |= (state?1:0)<<i;
			SetButtons(ButtonState);
			break;
		}
	}
}

void SoftIO::keyPressEvent(QKeyEvent *event)
{
	if(event->isAutoRepeat())
		return;
	uint16_t state=ButtonState;
	/* a through g is top row starting on the left
	 * left alt is top right
	 * right alt is bottom left
	 * h through ; is the bottom row starting on the left
	 * Same with the keyboard rows above and below those, this is so
	 * your left hand can be one row higher as a reminder that it is
	 * the upper row of buttons.
	 */
	//switch(event->key())
	// nativeVirtualKey is required to get left and right alt keys
	switch(event->nativeVirtualKey())
	{
	case 'q':
	case 'a':
	case 'z':
		ButtonState |= 1;
		break;
	case 'w':
	case 's':
	case 'x':
		ButtonState |= 2;
		break;
	case 'e':
	case 'd':
	case 'c':
		ButtonState |= 4;
		break;
	case 'r':
	case 'f':
	case 'v':
		ButtonState |= 8;
		break;
	case 't':
	case 'g':
	case 'b':
	case XK_Alt_L:
		ButtonState |= 0x10;
		break;
	case 'y':
	case 'h':
	case 'n':
	case XK_Alt_R:
		ButtonState |= 0x20;
		break;
	case 'u':
	case 'j':
	case 'm':
		ButtonState |= 0x40;
		break;
	case 'i':
	case 'k':
	case ',':
		ButtonState |= 0x80;
		break;
	case 'o':
	case 'l':
	case '.':
		ButtonState |= 0x100;
		break;
	case 'p':
	case ';':
	case '/':
		ButtonState |= 0x200;
		break;
	}
	if(state!=ButtonState)
	{
		SetButtons(ButtonState);
		UpdateButtons(state);
	}
}

void SoftIO::keyReleaseEvent(QKeyEvent *event)
{
	if(event->isAutoRepeat())
		return;
	uint16_t state=ButtonState;
	switch(event->nativeVirtualKey())
	{
	case 'q':
	case 'a':
	case 'z':
		ButtonState &= ~1;
		break;
	case 'w':
	case 's':
	case 'x':
		ButtonState &= ~2;
		break;
	case 'e':
	case 'd':
	case 'c':
		ButtonState &= ~4;
		break;
	case 'r':
	case 'f':
	case 'v':
		ButtonState &= ~8;
		break;
	case 't':
	case 'g':
	case 'b':
	case XK_Alt_L:
		ButtonState &= ~0x10;
		break;
	case 'y':
	case 'h':
	case 'n':
	case XK_Alt_R:
		ButtonState &= ~0x20;
		break;
	case 'u':
	case 'j':
	case 'm':
		ButtonState &= ~0x40;
		break;
	case 'i':
	case 'k':
	case ',':
		ButtonState &= ~0x80;
		break;
	case 'o':
	case 'l':
	case '.':
		ButtonState &= ~0x100;
		break;
	case 'p':
	case ';':
	case '/':
		ButtonState &= ~0x200;
		break;
	}
	if(state!=ButtonState)
	{
		SetButtons(ButtonState);
		UpdateButtons(state);
	}
}

void SoftIO::UpdateButtons(uint16_t was)
{
	uint16_t differs=was^ButtonState;
	uint16_t bit;
	for(int i=0; i<10; ++i)
	{
		bit=1<<i;
		if(differs & bit)
		{
			Buttons[i]->setCheckState(ButtonState & bit ?
				Qt::Checked : Qt::Unchecked);
		}
	}
}
