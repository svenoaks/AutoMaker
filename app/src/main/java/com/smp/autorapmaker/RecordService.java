package com.smp.autorapmaker;

import android.app.IntentService;
import android.content.Intent;
import android.content.Context;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.AudioTrack;
import android.util.Log;

import java.io.BufferedInputStream;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.List;
import java.util.concurrent.CountDownLatch;

import de.greenrobot.event.EventBus;
import edu.cuny.qc.speech.AuToBI.core.Region;
import edu.cuny.qc.speech.AuToBI.core.WavData;
import edu.cuny.qc.speech.AuToBI.core.syllabifier.Syllabifier;
import edu.cuny.qc.speech.AuToBI.core.syllabifier.VillingSyllabifier;
import vavi.sound.pcm.resampling.ssrc.SSRC;

import static android.media.MediaRecorder.AudioSource.*;
import static android.media.AudioFormat.*;

/**
 * An {@link IntentService} subclass for handling asynchronous task requests in
 * a service on a separate handler thread.
 * <p/>
 * TODO: Customize class - update intent actions, extra parameters and static
 * helper methods.
 */
public class RecordService extends IntentService
{
    // TODO: Rename actions, choose action names that describe tasks that this
    // IntentService can perform, e.g. ACTION_FETCH_NEW_ITEMS
    private static final String ACTION_RECORD = "com.smp.autorapmaker.action.FOO";
    private static final String ACTION_BAZ = "com.smp.autorapmaker.action.BAZ";

    // TODO: Rename parameters
    private static final String EXTRA_PARAM1 = "com.smp.autorapmaker.extra.PARAM1";
    private static final String EXTRA_PARAM2 = "com.smp.autorapmaker.extra.PARAM2";

    /**
     * Starts this service to perform action Foo with the given parameters. If
     * the service is already performing a task this action will be queued.
     *
     * @see IntentService
     */
    // TODO: Customize helper method
    public static void startActionRecord(Context context)
    {
        Intent intent = new Intent(context, RecordService.class);
        intent.setAction(ACTION_RECORD);
        //intent.putExtra(EXTRA_PARAM1, param1);
        //intent.putExtra(EXTRA_PARAM2, param2);
        context.startService(intent);
    }

    /**
     * Starts this service to perform action Baz with the given parameters. If
     * the service is already performing a task this action will be queued.
     *
     * @see IntentService
     */
    // TODO: Customize helper method
    public static void startActionBaz(Context context, String param1, String param2)
    {
        Intent intent = new Intent(context, RecordService.class);
        intent.setAction(ACTION_BAZ);
        intent.putExtra(EXTRA_PARAM1, param1);
        intent.putExtra(EXTRA_PARAM2, param2);
        context.startService(intent);
    }

    public RecordService()
    {
        super("RecordService");
    }

    @Override
    protected void onHandleIntent(Intent intent)
    {
        if (intent != null)
        {
            final String action = intent.getAction();
            if (ACTION_RECORD.equals(action))
            {
                final String param1 = intent.getStringExtra(EXTRA_PARAM1);
                final String param2 = intent.getStringExtra(EXTRA_PARAM2);
                handleActionRecord(param1, param2);
            } else if (ACTION_BAZ.equals(action))
            {
                final String param1 = intent.getStringExtra(EXTRA_PARAM1);
                final String param2 = intent.getStringExtra(EXTRA_PARAM2);
                handleActionBaz(param1, param2);
            }
        }
    }

    /**
     * Handle action Foo in the provided background thread with the provided
     * parameters.
     */
    private void handleActionRecord(String param1, String param2)
    {
        latch = new CountDownLatch(1);
        doRecord();
    }

    public void stopRecording()
    {
        recording = false;
        synchronized (recorderLock)
        {
            recorder.stop();
        }
    }

    @Override
    public void onDestroy()
    {
        EventBus.getDefault().unregister(this);
        super.onDestroy();
    }

    @Override
    public void onCreate()
    {
        super.onCreate();
        EventBus.getDefault().register(this);
        recorderLock = new Object();
    }

    public void onEvent(StopRecordingEvent event)
    {
        stopRecording();
        latch.countDown();
    }

    private void processBytes()
    {


        byte[] recorded = recordedBytes.toByteArray();
        byte[] beat = createBeatBytesForBeat(beatNo);

        WavData wavData = WavData.fromBytes(recorded, CHANNELS, SAMPLE_RATE_RECORDING, BITS);
        Syllabifier syl = new VillingSyllabifier();
        List<Region> regions = syl.generatePseudosyllableRegions(wavData);
        double[] regDouble = new double[regions.size() * 2];
        for (Region r : regions)
        {
            Log.i("SOUNDPROCESS", "REAL TIME:" + r.toString());
        }
        for (int i = 0; i < regions.size() * 2 - 1; i += 2)
        {
            Region r = regions.get(i / 2);
            regDouble[i] = r.getStart();
            regDouble[i + 1] = r.getEnd();
        }

        byte[] recordedResampled = resampleRecording(recorded);

        byte[] processed = NativeInterface.processAudio(recordedResampled, recordedResampled.length, SAMPLE_RATE_BEAT,
                regDouble, regDouble.length, beat, beat.length, SAMPLE_RATE_RECORDING);

        AudioTrack test = new AudioTrack(AudioManager.STREAM_MUSIC, SAMPLE_RATE_BEAT,
                AudioFormat.CHANNEL_OUT_STEREO, ENCODING, processed.length, AudioTrack.MODE_STATIC);

        test.write(processed, 0, processed.length);

        test.play();
    }

    private byte[] resampleRecording(byte[] recorded)
    {
        InputStream in = new ByteArrayInputStream(recorded);
        ByteArrayOutputStream out = new ByteArrayOutputStream(recorded.length);

        SSRC ssrc = new SSRC();

        try
        {
            ssrc.upsample(in, out, 1, 2, 2, SAMPLE_RATE_RECORDING, SAMPLE_RATE_BEAT, 3, recorded.length, false, 0);
        } catch (IOException e)
        {
            e.printStackTrace();
        }
        return out.toByteArray();
    }

    private byte[] createBeatBytesForBeat(int beatNo)
    {
        InputStream stream = null;
        switch (beatNo)
        {
            case 1:
                stream = getResources().openRawResource(R.raw.rapbeat1);
                break;
        }
        return Utils.prepareBytes(new BufferedInputStream(stream));
    }

    private void doRecord()
    {
        initAudioRecorder();
        recordedBytes = new ByteArrayOutputStream(MAXIMUM_BUFFER_SIZE);
        recording = true;
        recorder.startRecording();

        pollForBytes(false);

        try
        {
            latch.await();
        } catch (InterruptedException e)
        {
            e.printStackTrace();
        }

        pollForBytes(true);
        recorder.release();

        processBytes();
    }

    private void pollForBytes(boolean finishing)
    {
        while (recording || finishing)
        {
            synchronized (recorderLock)
            {
                byte[] chunk = new byte[CHUNK_SIZE];
                int theseBytes = recorder.read(chunk, 0, CHUNK_SIZE);
                if (theseBytes > 0)
                {
                    bytesRead += theseBytes;
                    recordedBytes.write(chunk, 0, theseBytes);
                }
                else if (finishing)
                    break;
            }
        }
    }
    private int beatNo = 1;
    private Object recorderLock;
    private ByteArrayOutputStream recordedBytes;
    private volatile int bytesRead;
    private volatile boolean recording;
    private static final int SAMPLE_RATE_RECORDING = 16000;
    private static final int SAMPLE_RATE_BEAT = 44100;
    private static final int BITS = 16;
    private static final int CHANNELS = 1;
    private static final int CHANNEL_CONFIG = CHANNEL_IN_MONO;
    private static final int ENCODING = ENCODING_PCM_16BIT;
    private static final int MAXIMUM_BUFFER_SIZE = 2000000;
    private static final int CHUNK_SIZE = 4096;
    private static int BUFFER_SIZE = 32000;
    private AudioRecord recorder;
    private CountDownLatch latch;

    private void initAudioRecorder()
    {
        while(AudioRecord.getMinBufferSize(SAMPLE_RATE_RECORDING, CHANNEL_CONFIG, ENCODING)
                > BUFFER_SIZE)
        {
            BUFFER_SIZE *= 2;
        }
        recorder = new AudioRecord(MIC, SAMPLE_RATE_RECORDING, CHANNEL_CONFIG,
                ENCODING_PCM_16BIT, BUFFER_SIZE);
    }

    /**
     * Handle action Baz in the provided background thread with the provided
     * parameters.
     */
    private void handleActionBaz(String param1, String param2)
    {
        // TODO: Handle action Baz
        throw new UnsupportedOperationException("Not yet implemented");
    }
}
