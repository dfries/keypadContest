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

#ifndef _SLOT_OWNER_H
#define _SLOT_OWNER_H

#include <QObject>

/* Receives a signal and adds the an object pointer to it when calling its
 * signal.
 */
class SlotOwner : public QObject
{
	Q_OBJECT
public:
	SlotOwner(QObject *obj, QObject *parent=NULL);
public slots:
	void SlotA() { SignalA(Obj); }
	void SlotA(bool value) { SignalA(value, Obj); }
	void SlotA(int value) { SignalA(value, Obj); }
signals:
	void SignalA(QObject *obj);
	void SignalA(int value, QObject *obj);
private:
	QObject *Obj;
};

#endif // _SLOT_OWNER_H
