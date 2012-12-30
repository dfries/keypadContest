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

#ifndef __LED_WIDGET_H
#define __LED_WIDGET_H

#include <QWidget>
#include <QTimer>
#include <QTime>

class LEDWidget : public QWidget
{
	Q_OBJECT
public:
	LEDWidget(QWidget *parent=NULL);
	// There are three output color, high, low, or in between if it
	// is frequently being turned on and off.
	void SetOn(bool on);
	bool GetOn() const { return On; }
private slots:
	void Timedout();
protected:
	void paintEvent(QPaintEvent *event);
private:
	// last requested state
	bool On;
	// displayed intensity, based on how often On changes
	unsigned char Intensity;
	// number of times requested to be on and off
	int ReqOn, ReqOff;
	QTimer Timer;
	QTime Time;
};

#endif // __LED_WIDGET_H
