package com.smp.autorapmaker;

import java.io.BufferedInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;

/**
 * Created by Steve on 1/25/15.
 */

public class Utils
{
    public static final int DEFAULT_STREAM_SIZE =  2000000;
    public static byte[] prepareBytes(BufferedInputStream inputStream)
    {
        ByteArrayOutputStream outputStream = new ByteArrayOutputStream(DEFAULT_STREAM_SIZE);
        long total = 0;
        int read = 0;
        byte[] bytes = new byte[4096];
        try
        {
            while ((read = inputStream.read(bytes)) != -1)
            {
                total += read;
                outputStream.write(bytes, 0, read);
            }
            return outputStream.toByteArray();
        } catch (IOException e)
        {
            e.printStackTrace();
        } finally
        {
            if (inputStream != null)
            {
                try
                {
                    inputStream.close();
                } catch (IOException e)
                {
                    e.printStackTrace();
                }
            }
            if (outputStream != null)
            {
                try
                {
                    outputStream.flush();
                    outputStream.close();
                } catch (IOException e)
                {
                    e.printStackTrace();
                }

            }
        }
        return null;
    }
}
