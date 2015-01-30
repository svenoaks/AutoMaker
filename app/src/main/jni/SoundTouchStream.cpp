

#include <jni.h>
#include <android/log.h>
#include <queue>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <utility>
#include <memory>

#include "SoundTouchStream.h"

using namespace soundtouch;
using namespace std;

SoundTouchStream::SoundTouchStream() {
	sampleRate = bytesPerSample = 0;
}

/*SoundTouchStream::SoundTouchStream(const SoundTouchStream& other) {
	sampleRate = bytesPerSample = 0;
}*/

SoundTouchStream::SoundTouchStream(int channels, int sampleRate,
                                    int bytesPerSample, float tempo, float pitchSemi)
{
    setup(channels, sampleRate, bytesPerSample, tempo, pitchSemi);
}

void SoundTouchStream::transformSamples(vector<jbyte>& input)
{
    const static int MAX_SIZE = 2048;

    auto it = input.begin();
    while (it < input.end())
    {
         const auto length = min(MAX_SIZE, distance(it, input.end()));
         const int BUFF_SIZE = length / bytesPerSample;
         unique_ptr<SAMPLETYPE[]> fBufferIn(new SAMPLETYPE[BUFF_SIZE]);
         convertInput(&(*it), fBufferIn, BUFF_SIZE, bytesPerSample);
         process(fBufferIn, BUFF_SIZE, false); //audio is ongoing.
         it += length;
    }
    const int BUFF_SIZE = MAX_SIZE / bytesPerSample;
    unique_ptr<SAMPLETYPE[]> fBufferIn(new SAMPLETYPE[BUFF_SIZE]);
    process(fBufferIn, BUFF_SIZE, true); //all bytes have been placed in the soundTouch pipe.

}
vector<jbyte> SoundTouchStream::getStream() {
	return bufferOut;
}

int SoundTouchStream::getSampleRate() {
	return sampleRate;
}

int SoundTouchStream::getBytesPerSample() {
	return bytesPerSample;
}

void SoundTouchStream::setSampleRate(int sampleRate) {
	SoundTouch::setSampleRate(sampleRate);
	this->sampleRate = sampleRate;
}

void SoundTouchStream::setBytesPerSample(int bytesPerSample) {
	this->bytesPerSample = bytesPerSample;
}

uint SoundTouchStream::getChannels() {
	return channels;
}



int SoundTouchStream::process(const unique_ptr<SAMPLETYPE[]>& fBufferIn, const int BUFF_SIZE, bool finishing) {
	const uint channels = getChannels();
	const int buffSizeSamples = BUFF_SIZE / channels;
	const int bytesPerSample = getBytesPerSample();

	int nSamples = BUFF_SIZE / channels;

	int processed = 0;

	if (finishing) {
		flush();
	} else {
		putSamples(fBufferIn.get(), nSamples);
	}

	do {
		nSamples = receiveSamples(fBufferIn.get(), buffSizeSamples);
		processed += write(fBufferIn, nSamples * channels,
				bytesPerSample);
	} while (nSamples != 0);

	return processed;
}

void* SoundTouchStream::getConvBuffer(int sizeBytes) {
	int convBuffSize = (sizeBytes + 15) & -8;
	// round up to following 8-byte bounday
	char *convBuff = new char[convBuffSize];
	return convBuff;
}

int SoundTouchStream::write(const unique_ptr<float[]>& bufferIn, int numElems,
		int bytesPerSample) {
	int numBytes;

	int oldSize = bufferOut.size();

	if (numElems == 0)
		return 0;

	numBytes = numElems * bytesPerSample;
	short *temp = (short*) getConvBuffer(numBytes);

	switch (bytesPerSample) {
	case 1: {
		unsigned char *temp2 = (unsigned char *) temp;
		for (int i = 0; i < numElems; i++) {
			temp2[i] = (unsigned char) saturate(bufferIn[i] * 128.0f + 128.0f,
					0.0f, 255.0f);
		}
		break;
	}

	case 2: {
		short *temp2 = (short *) temp;
		for (int i = 0; i < numElems; i++) {
			short value = (short) saturate(bufferIn[i] * 32768.0f, -32768.0f,
					32767.0f);
			temp2[i] = value;
		}
		break;
	}

	case 3: {
		char *temp2 = (char *) temp;
		for (int i = 0; i < numElems; i++) {
			int value = saturate(bufferIn[i] * 8388608.0f, -8388608.0f,
					8388607.0f);
			*((int*) temp2) = value;
			temp2 += 3;
		}
		break;
	}

	case 4: {
		int *temp2 = (int *) temp;
		for (int i = 0; i < numElems; i++) {
			int value = saturate(bufferIn[i] * 2147483648.0f, -2147483648.0f,
					2147483647.0f);
			temp2[i] = value;
		}
		break;
	}
	default:
		//should throw?
		break;
	}
	for (int i = 0; i < numBytes / 2; ++i) {
		bufferOut.push_back(temp[i] & 0xff);
		bufferOut.push_back((temp[i] >> 8) & 0xff);
	}
	delete[] temp;
	temp = NULL;
	return bufferOut.size() - oldSize;
}


void SoundTouchStream::setup(int channels, int sampleRate,
		int bytesPerSample, float tempoChange, float pitchSemi) {
	setBytesPerSample(bytesPerSample);

	setSampleRate(sampleRate);
	setChannels(channels);

	setTempo(tempoChange);
	setPitchSemiTones(pitchSemi);
	setRateChange(0);

	setSetting(SETTING_USE_QUICKSEEK, false);
	setSetting(SETTING_USE_AA_FILTER, true);
	setSetting(SETTING_SEQUENCE_MS, 40);
    setSetting(SETTING_SEEKWINDOW_MS, 15);
    setSetting(SETTING_OVERLAP_MS, 8);

}

void SoundTouchStream::convertInput(const jbyte* input, const unique_ptr<float[]>& output, const int BUFF_SIZE,
		int bytesPerSample) {
	switch (bytesPerSample) {
	case 1: {
		unsigned char *temp2 = (unsigned char*) input;
		double conv = 1.0 / 128.0;
		for (int i = 0; i < BUFF_SIZE; i++) {
			output[i] = (float) (temp2[i] * conv - 1.0);
		}
		break;
	}
	case 2: {
		short *temp2 = (short*) input;
		double conv = 1.0 / 32768.0;
		for (int i = 0; i < BUFF_SIZE; i++) {
			short value = temp2[i];
			output[i] = (float) (value * conv);
		}
		break;
	}
	case 3: {
		char *temp2 = (char *) input;
		double conv = 1.0 / 8388608.0;
		for (int i = 0; i < BUFF_SIZE; i++) {
			int value = *((int*) temp2);
			value = value & 0x00ffffff;             // take 24 bits
			value |= (value & 0x00800000) ? 0xff000000 : 0; // extend minus sign bits
			output[i] = (float) (value * conv);
			temp2 += 3;
		}
		break;
	}
	case 4: {
		int *temp2 = (int *) input;
		double conv = 1.0 / 2147483648.0;
		assert(sizeof(int) == 4);
		for (int i = 0; i < BUFF_SIZE; i++) {
			int value = temp2[i];
			output[i] = (float) (value * conv);
		}
		break;
	}
	}
}
inline int SoundTouchStream::saturate(float fvalue, float minval, float maxval) {
	if (fvalue > maxval) {
		fvalue = maxval;
	} else if (fvalue < minval) {
		fvalue = minval;
	}
	return (int) fvalue;
}




