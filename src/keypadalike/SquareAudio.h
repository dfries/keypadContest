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

#ifndef _SQUARE_AUDIO_H
#define _SQUARE_AUDIO_H

#include <QVector>
#include <sys/time.h>

class QIODevice;
class QAudioOutput;

/* Given a speaker connected between two microcontroller pins, generate
 * audio for the sound card to playback.
 */
class SquareAudio
{
public:
	SquareAudio();
	~SquareAudio();
	void SetPins(bool pin0, bool pin1);
private:
	// The audio device is created on the first SetPins call, but if that
	// fails, don't keep trying to open the device, just ignore any
	// more SetPins request.  This stores if the first open request has
	// been attempted.
	bool FirstTry;
	QAudioOutput *Audio;
	QIODevice *IODevice;
	QVector<int16_t> Samples;
	// The current value of the pins.
	int16_t Value;
	struct timeval LastWrite;
};

#endif // _SQUARE_AUDIO_H
