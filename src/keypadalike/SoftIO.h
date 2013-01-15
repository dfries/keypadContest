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
#ifndef _SOFT_IO_H
#define _SOFT_IO_H

#include <QWidget>
class QCheckBox;
class LEDWidget;

/* Instead of the hardware buttons and LEDs, do software push buttons and
 * colored circles in software.
 */
class SoftIO : public QWidget
{
	Q_OBJECT
	enum {BUTTON_COUNT=10};
public:
	SoftIO();

public slots:
	// like the hardware bit 0 to 9 is, 0 top left to top right,
	// then bottom left to bottom right
	void SetLEDs(uint16_t led);
signals:
	void SetButtons(uint16_t button);
private slots:
	void Clicked(int state, QObject *sender);
protected:
	void keyPressEvent(QKeyEvent *event);
	void keyReleaseEvent(QKeyEvent *event);

private:
	void UpdateButtons(uint16_t was);
	LEDWidget *LEDs[BUTTON_COUNT];
	QCheckBox *Buttons[BUTTON_COUNT];
	uint16_t LEDState;
	uint16_t ButtonState;
};

#endif //_SOFT_IO_H
