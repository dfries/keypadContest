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

#include "SquareAudio.h"
#include <QIODevice>
#include <QtMultimediaKit/QAudioOutput>
#include <QtMultimediaKit/QAudioFormat>
#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include "util.h"

using namespace std;

SquareAudio::SquareAudio() :
	FirstTry(true),
	Audio(NULL),
	IODevice(NULL),
	Value(0)
{
	LastWrite.tv_sec=0;
	LastWrite.tv_usec=0;
}

SquareAudio::~SquareAudio()
{
	delete Audio;
	delete IODevice;
}

void SquareAudio::SetPins(bool pin0, bool pin1)
{
	const short range=2048;
	int16_t s;
	if(pin0 == pin1)
		s=0;
	else if(pin0)
		s=range;
	else
		s=-range;
	int16_t prev=Value;
	Value=s;
	const int freq=8000;
	const int sample_bytes=2;
	// buffer size in seconds
	const double buffer=.050;
	Samples.resize((int)(buffer*freq)*sample_bytes);

	if(!IODevice || !Audio)
	{
		// Optimization, if it isn't initialized, and the value is
		// still zero, ignore it to avoid starting audio when only
		// the LEDs are being set or the buttons are being read and
		// this is being called just because the register is being
		// written to when the program isn't intending to produce
		// audio.
		if(!prev && !s)
			return;

		// Initialize only once.
		if(!FirstTry)
			return;

		FirstTry=false;

		// The calling thread should have been set to run on only
		// one thread, but audio creates a new thread inheriting
		// the current affinity.  Allow the audio thread to run on any
		// cpu.
		cpu_set_t mask, main;
		sched_getaffinity(0, sizeof(mask), &mask);
		// copy from the main thread
		sched_getaffinity(getpid(), sizeof(main), &main);
		sched_setaffinity(0, sizeof(main), &main);

		QAudioFormat format;
		format.setFrequency(freq);
		format.setChannels(1);
		format.setSampleSize(16);
		format.setCodec("audio/pcm");
		format.setByteOrder(QAudioFormat::LittleEndian);
		format.setSampleType(QAudioFormat::SignedInt);

		QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
		if(!info.isFormatSupported(format))
		{
			cerr << "audio format unsupported\n";
			return;
		}

		Audio=new QAudioOutput(format);
		Audio->setBufferSize(sample_bytes*(int)(buffer*freq));
		IODevice=Audio->start();

		sched_setaffinity(0, sizeof(mask), &mask);

		if(!IODevice)
			return;
	}

	struct timeval now;
	gettimeofday(&now, NULL);
	double delta=now-LastWrite;

	if(delta > buffer)
		delta = buffer;
	// skip extremely small time values as the register is written for
	// more reasons than just audio
	if(prev == Value && delta < .001)
		return;

	int count=(int)(delta*freq+.5);
	if(!count)
		return;

	LastWrite=now;
	
	// This is audio until now which uses the previous speaker setting.
	for(int i=0; i<count; ++i)
		Samples[i]=Value;
	
	IODevice->write((char*)Samples.data(), count*sample_bytes);
	//printf("time delta %8.6f, %4u samples written\n", delta, count);
}
