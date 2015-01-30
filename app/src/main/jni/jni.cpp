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
vector<pair<double, double>> posOfBeatPoints();
void mixBytes(jbyte&, const jbyte&);
int timeToSample(double);
vector<vector<jbyte>> timeStretchSamples(const vector<pair<double, double>>& posOfBeats,
                                            const vector<jbyte>& recording,
                                            const vector<jdouble> regions);

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

    beat.reserve(beat.size() * 2);
    beat.insert(beat.end(), beat.begin(), beat.end());
    vector<pair<double, double>> posOfBeats = posOfBeatPoints();
    vector<vector<jbyte>> modifiedRecordings = timeStretchSamples(posOfBeats, rec, regions);
    LOGI("returned modified");
    for (unsigned int i = 0; i < posOfBeats.size() &&
        i < modifiedRecordings.size(); ++i)
    {
        unsigned int beatStart = timeToSample(posOfBeats[i].first);
        unsigned int beatEnd = timeToSample(posOfBeats[i].first + posOfBeats[i].second);

        auto& stream = modifiedRecordings.at(i);
        LOGI("got stream");
        //LOGI("%d beatSTart, %d beatEnd, %d recStart, %d recEnd, %d rec size, %d regions size, %d i", beatStart, beatEnd, recStart,
        //recEnd, rec.size(), regions.size(), i);
        for (unsigned int j = beatStart, k = 0; j < beatEnd && k < stream.size(); ++j, ++k)
        {
            mixBytes(beat.at(j), stream.at(k));
        }
        LOGI("mixed");

    }
    //LOGI("%d", modifiedRecordings[1].getStream().size());
    return beat;
}
vector<vector<jbyte>> timeStretchSamples(const vector<pair<double, double>>& posOfBeats,
                                            const vector<jbyte>& recording,
                                            const vector<jdouble> regions)
{
    vector<vector<jbyte>> result;

    for (unsigned int i = 0; i < posOfBeats.size() &&
            i < regions.size() / 2; ++i)
    {
    LOGI("top of loop");
        int beatStart = timeToSample(posOfBeats[i].first);
        int beatEnd = timeToSample(posOfBeats[i].first + posOfBeats[i].second);
        int recStart = timeToSample(regions[i * 2]);
        int recEnd = timeToSample(regions[i * 2 + 1]);

        if (beatStart % 2 == 0 && recStart % 2 != 0) recStart -= 1;
        if (beatStart % 2 != 0 && recStart % 2 == 0) recStart -= 1;

        int beatTime = beatEnd - beatStart;
        int recTime = recEnd - recStart;



        float timeRatio = (float)recTime / beatTime;

         SoundTouchStream stream(2, 44100, 2, timeRatio, 0.0f);
         LOGI("after STREAM CONST %d beatTime, %d recTime, %f timeRatio", beatTime, recTime, timeRatio);
         vector<jbyte> regionSamples{recording.begin() + recStart,
                                        min(recording.begin() + recEnd, recording.end())};
         //LOGI("%d", regionSamples.size());
         stream.transformSamples(regionSamples);
         //LOGI("%d", stream.getStream().size());
         LOGI("after transform");
         result.push_back(stream.getStream());
         LOGI("after push");
    }
    LOGI("after loop");
    //LOGI("%d", result[1].getStream().size());
    return result;

}
vector<jbyte> monoToStereo(vector<jbyte>& input)
{
    vector<jbyte> output(input.size() * 2);

    for (unsigned int i = 0; i < input.size() - 1; i += 2) {
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

vector<pair<double, double>> posOfBeatPoints()
{
    const static double bpm = 75;
    double sixteenth = 60 / bpm / 4;

    vector<pair<double, double>> pos;

    pos.push_back(pair<double, double>(sixteenth * 2, sixteenth * 1));
    pos.push_back(pair<double, double>(sixteenth * 3, sixteenth * 2));
    pos.push_back(pair<double, double>(sixteenth * 5, sixteenth * 1));
    pos.push_back(pair<double, double>(sixteenth * 6, sixteenth * 1));
    pos.push_back(pair<double, double>(sixteenth * 7, sixteenth * 1));
    pos.push_back(pair<double, double>(sixteenth * 8, sixteenth * 1));
    pos.push_back(pair<double, double>(sixteenth * 9, sixteenth * 1));
    pos.push_back(pair<double, double>(sixteenth * 10, sixteenth * 1));
    pos.push_back(pair<double, double>(sixteenth * 11, sixteenth * 1));
    pos.push_back(pair<double, double>(sixteenth * 12, sixteenth * 1));
    pos.push_back(pair<double, double>(sixteenth * 13, sixteenth * 1));
    pos.push_back(pair<double, double>(sixteenth * 14, sixteenth * 1));
    pos.push_back(pair<double, double>(sixteenth * 15, sixteenth * 1));
    pos.push_back(pair<double, double>(sixteenth * 16, sixteenth * 2));
    pos.push_back(pair<double, double>(sixteenth * 18, sixteenth * 1));
    pos.push_back(pair<double, double>(sixteenth * 19, sixteenth * 1));
    pos.push_back(pair<double, double>(sixteenth * 20, sixteenth * 1));
    pos.push_back(pair<double, double>(sixteenth * 21, sixteenth * 1));
    pos.push_back(pair<double, double>(sixteenth * 22, sixteenth * 1));
    pos.push_back(pair<double, double>(sixteenth * 23, sixteenth * 1));
    pos.push_back(pair<double, double>(sixteenth * 24, sixteenth * 1));
    pos.push_back(pair<double, double>(sixteenth * 25, sixteenth * 1));
    pos.push_back(pair<double, double>(sixteenth * 26, sixteenth * 1));
    pos.push_back(pair<double, double>(sixteenth * 27, sixteenth * 1));
    pos.push_back(pair<double, double>(sixteenth * 28, sixteenth * 1));
    pos.push_back(pair<double, double>(sixteenth * 29, sixteenth * 1));
    pos.push_back(pair<double, double>(sixteenth * 30, sixteenth * 1));
    pos.push_back(pair<double, double>(sixteenth * 31, sixteenth * 2));
    pos.push_back(pair<double, double>(sixteenth * 33, sixteenth * 1));
    pos.push_back(pair<double, double>(sixteenth * 34, sixteenth * 1));
    pos.push_back(pair<double, double>(sixteenth * 35, sixteenth * 1));
        pos.push_back(pair<double, double>(sixteenth * 36, sixteenth * 1));
        pos.push_back(pair<double, double>(sixteenth * 37, sixteenth * 1));
        pos.push_back(pair<double, double>(sixteenth * 38, sixteenth * 1));
        pos.push_back(pair<double, double>(sixteenth * 39, sixteenth * 1));
        pos.push_back(pair<double, double>(sixteenth * 40, sixteenth * 1));
        pos.push_back(pair<double, double>(sixteenth * 41, sixteenth * 1));
        pos.push_back(pair<double, double>(sixteenth * 42, sixteenth * 1));
        pos.push_back(pair<double, double>(sixteenth * 43, sixteenth * 1));
        pos.push_back(pair<double, double>(sixteenth * 44, sixteenth * 2));
        pos.push_back(pair<double, double>(sixteenth * 46, sixteenth * 1));
        pos.push_back(pair<double, double>(sixteenth * 47, sixteenth * 1));
        pos.push_back(pair<double, double>(sixteenth * 48, sixteenth * 1));
        pos.push_back(pair<double, double>(sixteenth * 49, sixteenth * 1));
        pos.push_back(pair<double, double>(sixteenth * 50, sixteenth * 1));
        pos.push_back(pair<double, double>(sixteenth * 51, sixteenth * 1));
        pos.push_back(pair<double, double>(sixteenth * 52, sixteenth * 1));
        pos.push_back(pair<double, double>(sixteenth * 53, sixteenth * 1));
        pos.push_back(pair<double, double>(sixteenth * 54, sixteenth * 1));
        pos.push_back(pair<double, double>(sixteenth * 55, sixteenth * 1));
        pos.push_back(pair<double, double>(sixteenth * 56, sixteenth * 1));
        pos.push_back(pair<double, double>(sixteenth * 57, sixteenth * 1));
        pos.push_back(pair<double, double>(sixteenth * 58, sixteenth * 2));
        pos.push_back(pair<double, double>(sixteenth * 59, sixteenth * 1));
        pos.push_back(pair<double, double>(sixteenth * 60, sixteenth * 2));



    return pos;
}