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

#include "LEDWidget.h"
#include <QPainter>
#include <algorithm>

const int Interval=80;

LEDWidget::LEDWidget(QWidget *parent) :
	QWidget(parent),
	On(false),
	Intensity(0),
	ReqOn(0),
	ReqOff(0)
{
	setMinimumSize(20, 20);
	Timer.setInterval(Interval);
	Timer.setSingleShot(true);
	connect(&Timer, SIGNAL(timeout()), SLOT(Timedout()));
	Time.start();
	Time.addMSecs(Interval);
}

void LEDWidget::SetOn(bool on)
{
	On=on;
	if(Time.elapsed() < Interval)
	{
		if(on)
			++ReqOn;
		else
			++ReqOff;
		if(!Timer.isActive())
			Timer.start();
	}
	else
	{
		Time.start();
		ReqOn=ReqOff=0;
		Intensity=255*On;
		update();
	}
}

void LEDWidget::paintEvent(QPaintEvent *event)
{
	QColor fg(Intensity, 0, 0);
	QPainter painter(this);
	painter.setBrush(fg);
	int diameter=std::min(width(), height()) *3/4;
	int radius=diameter/2;
	// draw a centered circle
	painter.drawEllipse(width()/2-radius, height()/2-radius,
		diameter, diameter);
}

void LEDWidget::Timedout()
{
	if(ReqOn || ReqOff)
	{
		Intensity=255*ReqOn/(ReqOn+ReqOff);
		ReqOn=ReqOff=0;
	}
	else
	{
		Intensity=255*On;
	}
	Time.start();
	update();
}
