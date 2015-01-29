#include <jni.h>
#include <android/log.h>
#include <string>
#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <stack>
#include <map>
#include <utility>
#include "SoundTouchStream.h"

#define DLL_PUBLIC __attribute__ ((visibility ("default")))
#define LOGI(...)   __android_log_print((int)ANDROID_LOG_INFO, "SOUNDPROCESS", __VA_ARGS__)

using namespace std;

vector<jbyte> processBytes(vector<jbyte>&, vector<jbyte>&, vector<jdouble>&);
vector<jbyte> monoToStereo(vector<jbyte>&);
vector<double> posOfBeatPoints();
void mixBytes(jbyte&, const jbyte&);
int timeToSample(double);

extern "C" DLL_PUBLIC jstring Java_com_smp_autorapmaker_NativeInterface_testString(
		JNIEnv *env, jobject thiz)
{
    return env->NewStringUTF("NDK Sample Test !");
}

extern "C" DLL_PUBLIC jbyteArray Java_com_smp_autorapmaker_NativeInterface_processAudio(
		JNIEnv *env, jobject thiz,
		jbyteArray recorded, jint bytesRecorded, jint recordedSamplingRate,
		jdoubleArray syllableRegions, jint regionLen,
		jbyteArray beat, jint bytesBeat, jint beatSamplingRate)
{
    jbyte* recBuf = (jbyte*) env->GetByteArrayElements(recorded, nullptr);
    jbyte* beatBuf = (jbyte*) env->GetByteArrayElements(beat, nullptr);
    jdouble* regionBuf = (jdouble*) env->GetDoubleArrayElements(syllableRegions, nullptr);

    vector<jbyte> recordedBytes(recBuf, recBuf + bytesRecorded);
    vector<jbyte> beatBytes(beatBuf, beatBuf + bytesBeat);
    vector<jdouble> regions(regionBuf, regionBuf + regionLen);

    vector<jbyte> processed = processBytes(recordedBytes, beatBytes, regions);

    env->ReleaseByteArrayElements(recorded, recBuf, JNI_ABORT);
    env->ReleaseByteArrayElements(beat, beatBuf, JNI_ABORT);
    env->ReleaseDoubleArrayElements(syllableRegions, regionBuf, JNI_ABORT);

    jbyteArray result = env->NewByteArray(processed.size());
    env->SetByteArrayRegion(result, 0, processed.size(), &processed[0]);
    return result;
}

vector<jbyte> processBytes(vector<jbyte>& recorded, vector<jbyte>& beat, vector<jdouble>& regions)
{
    vector<jbyte> rec = monoToStereo(recorded);
    vector<double> posOfBeats = posOfBeatPoints();
    for (int i = 0; i < posOfBeats.size() &&
        i < regions.size() / 2; ++i)
    {
        int beatStart = timeToSample(posOfBeats[i]);
        int beatEnd = timeToSample(posOfBeats[i] + 60.0 / 75 / 4);
        int recStart = timeToSample(regions[i * 2]);
        int recEnd = timeToSample(regions[i * 2 + 1]);

        if (beatStart % 2 == 0 && recStart % 2 != 0) recStart -= 1;
        if (beatStart % 2 != 0 && recStart % 2 == 0) recStart -= 1;
        //LOGI("%d beatSTart, %d beatEnd, %d recStart, %d recEnd, %d rec size, %d regions size, %d i", beatStart, beatEnd, recStart,
        //recEnd, rec.size(), regions.size(), i);
        for (int j = recStart, k = beatStart; j < recEnd && k < beatEnd; ++j, ++k)
        {
            mixBytes(beat.at(k), rec.at(j));
        }

    }
    LOGI("BEFORE STREAM CONST");
    SoundTouchStream stream(2, 44100, 2, 1.5f, 0.0f);
    LOGI("after STREAM CONST");
    stream.transformSamples(rec);
    LOGI("after transform");
    vector<jbyte> trans = stream.getStream();
    return trans;
}
vector<jbyte> monoToStereo(vector<jbyte>& input)
{
    vector<jbyte> output(input.size() * 2);

    for (int i = 0; i < input.size() - 1; i += 2) {
        output.at(i*2+0) = input.at(i);
        output.at(i*2+1) = input.at(i+1);
        output.at(i*2+2) = input.at(i);
        output.at(i*2+3) = input.at(i+1);
    }

    return output;
}

int timeToSample(double time)
{
    //bounds checking.
    //LOGI("TIME: %f", time);
    return time * 44100 * 2 * 2;
}

inline void mixBytes(jbyte& toReturn, const jbyte& toMix)
{
    int pre = (int) toReturn;
    int pre2 = (int) toMix;

    int inte = (pre + pre2) / 2;

    if (inte > 127) inte = 127;
    if (inte < -127) inte = -127;

    toReturn = (jbyte) inte;
}

vector<double> posOfBeatPoints()
{
    const static double bpm = 75;
    const static double sixteenth = 60 / bpm / 4;

    vector<double> pos;
     /*pos.push_back(sixteenth * 0);
        pos.push_back(sixteenth * 4);
        pos.push_back(sixteenth * 8);
        pos.push_back(sixteenth * 12);
        pos.push_back(sixteenth * 16);*/
   //pos.push_back(sixteenth * 0);

    pos.push_back(sixteenth * 0);
    pos.push_back(sixteenth * 4);
    pos.push_back(sixteenth * 5);
    pos.push_back(sixteenth * 7);
    pos.push_back(sixteenth * 9);
    pos.push_back(sixteenth * 11);
    pos.push_back(sixteenth * 13);
    pos.push_back(sixteenth * 15);
    pos.push_back(sixteenth * 17);
    pos.push_back(sixteenth * 19);
    pos.push_back(sixteenth * 20);
    pos.push_back(sixteenth * 22);
    pos.push_back(sixteenth * 24);
    /*pos.push_back(sixteenth * 19);
    pos.push_back(sixteenth * 21);
    pos.push_back(sixteenth * 23);
    pos.push_back(sixteenth * 25);
    pos.push_back(sixteenth * 29);
    pos.push_back(sixteenth * 31);*/
   /* pos.push_back(sixteenth * 13);
    pos.push_back(sixteenth * 14);
    pos.push_back(sixteenth * 15);*/
    return pos;
}