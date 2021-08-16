package com.example.jniffmpegstaticplay;

import android.Manifest;
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
                AudioPlayer audioPlayer = new AudioPlayer();
                String input = new File(Environment.getExternalStorageDirectory(), "jiarihaitan.mp3").getAbsolutePath();
                String output = new File(Environment.getExternalStorageDirectory(), "jiarihaitan.pcm").getAbsolutePath();
                audioPlayer.soundPlay(input, output);
            }
        });

        binding.netPlay.setOnClickListener(view -> {
            jniffPlayer.setDataSource(httpUrl);
            jniffPlayer.prepare();
        });
    }

    private void setPlayerListener() {
        jniffPlayer.setOnProgressListener(new JNIffPlayer.OnProgressListener() {
            @Override
            public void onProgress(int progress) {
                OLog.d("MainActivity, onProgress:"+ progress);
            }
        });

        jniffPlayer.setOnPrepareListener(new JNIffPlayer.OnPrepareListener() {
            @Override
            public void onPrepare() {
                jniffPlayer.start();
            }
        });

        jniffPlayer.setOnErrorListener(new JNIffPlayer.OnErrorListener() {
            @Override
            public void onError(int errorCode) {

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