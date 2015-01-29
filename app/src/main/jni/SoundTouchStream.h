#ifndef SOUNDTOUCHSTREAM_H
#define SOUNDTOUCHSTREAM_H

#include <jni.h>
#include <android/log.h>
#include <queue>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <memory>
#include "soundtouch/include/SoundTouch.h"

#define LOGI(...)   __android_log_print((int)ANDROID_LOG_INFO, "SOUNDPROCESS", __VA_ARGS__)

using namespace soundtouch;
using namespace std;

class SoundTouchStream: public SoundTouch {

private:
	vector<jbyte> bufferOut;
	int sampleRate;
	int bytesPerSample;

	void* getConvBuffer(int);
    int write(const unique_ptr<float[]>&, int, int);
    void setup(int, int, int, float, float);
    void convertInput(const jbyte*, const unique_ptr<float[]>&, int, int);
    inline int saturate(float, float, float);
    int process(const unique_ptr<SAMPLETYPE[]>&, int, bool);



public:

	const vector<jbyte>& getStream();

	void transformSamples(vector<jbyte>& input);

	int getSampleRate();

	int getBytesPerSample();

	void setSampleRate(int sampleRate);

	void setBytesPerSample(int bytesPerSample);

	uint getChannels();

	SoundTouchStream();

	SoundTouchStream(const SoundTouchStream& other);
	SoundTouchStream(int channels, int sampleRate, int bytesPerSample, float tempo, float pitchSemi);
};

#endif