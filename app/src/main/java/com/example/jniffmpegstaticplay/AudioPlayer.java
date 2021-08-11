package com.example.jniffmpegstaticplay;

public class AudioPlayer {
    static {
        System.loadLibrary("jniffmpegstaticplay");
    }

    public native void soundPlay(String input,String output);

}
