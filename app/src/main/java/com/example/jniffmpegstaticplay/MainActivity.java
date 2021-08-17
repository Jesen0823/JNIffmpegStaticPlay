package com.example.jniffmpegstaticplay;

import android.Manifest;
import android.content.Context;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.Environment;
import android.view.View;
import android.view.WindowManager;
import android.widget.SeekBar;

import androidx.appcompat.app.AppCompatActivity;

import com.example.jniffmpegstaticplay.databinding.ActivityMainBinding;
import com.example.jniffmpegstaticplay.utils.AppUtil;
import com.example.jniffmpegstaticplay.utils.OLog;

import java.io.File;

public class MainActivity extends AppCompatActivity implements SeekBar.OnSeekBarChangeListener {

    public static String httpUrl = "http://vjs.zencdn.net/v/oceans.mp4";

    private ActivityMainBinding binding;
    public JNIffPlayer jniffPlayer;
    private int progress;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // 常亮
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON,
                WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        binding.seekBar.setOnSeekBarChangeListener(this);

        jniffPlayer = new JNIffPlayer();
        jniffPlayer.setSurfaceView(binding.surfaceView);
        setPlayerListener();

        AppUtil.requestPermissions(MainActivity.this, Manifest.permission.READ_EXTERNAL_STORAGE);
        AppUtil.requestPermissions(MainActivity.this, Manifest.permission.WRITE_EXTERNAL_STORAGE);
        AppUtil.requestPermissions(MainActivity.this, Manifest.permission.RECORD_AUDIO);

        binding.btnPlay.setOnClickListener(new View.OnClickListener(){
            @Override
            public void onClick(View view) {
                File file = new File(Environment.getExternalStorageDirectory(),"innput.mp4");

                OLog.d("MainActivity, path:"+file.getAbsolutePath());
                jniffPlayer.setDataSource(file.getAbsolutePath());
                jniffPlayer.prepare();
            }
        });

        binding.btnAudio.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {

            }
        });

        binding.netPlay.setOnClickListener(view -> {
            jniffPlayer.setDataSource(httpUrl);
            jniffPlayer.prepare();
        });

        AudioManager myAudioMgr = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        String nativeSampleRate = myAudioMgr.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE);
        OLog.d("MainActivity" + "hardware support samplerate: " + nativeSampleRate);
    }

    private void setPlayerListener() {
        jniffPlayer.setOnProgressListener(new JNIffPlayer.OnProgressListener() {
            @Override
            public void onProgress(int progress) {
                OLog.d("MainActivity,from JNI, onProgress:" + progress);
            }
        });

        jniffPlayer.setOnPrepareListener(new JNIffPlayer.OnPrepareListener() {
            @Override
            public void onPrepare() {
                OLog.d("MainActivity,from JNI, prepare finish, now start");
                jniffPlayer.start();
            }
        });

        jniffPlayer.setOnErrorListener(new JNIffPlayer.OnErrorListener() {
            @Override
            public void onError(int errorCode) {
                OLog.d("MainActivity,from JNI, Error:" + errorCode);
            }
        });
    }

    @Override
    public void onProgressChanged(SeekBar seekBar, int i, boolean b) {

    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {

    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {

    }
}