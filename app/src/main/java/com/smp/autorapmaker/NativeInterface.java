package com.smp.autorapmaker;

import java.nio.ByteBuffer;

/**
 * Created by Steve on 1/25/15.
 */
public class NativeInterface
{
    public static synchronized final native String testString();
    public static synchronized final native byte[] processAudio(byte[] recorder, int bytesRecorded, int recordedResamplingRate,
                                                                double[] syllables, int regionLen,
                                                                byte[] beat, int bytesBeat, int beatResamplingRate);
}
