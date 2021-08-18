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
    private static final String TAG = "MainActivity";

    private ActivityMainBinding binding;
    public JNIffPlayer jniffPlayer;
    private boolean isTouch;
    private boolean isSeek;

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

                OLog.d(TAG + " path:" + file.getAbsolutePath());
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
        binding.pauseBtn.setOnClickListener(view -> {
            jniffPlayer.pause();
        });
        binding.resumeBtn.setOnClickListener(view -> {
            jniffPlayer.resume();
        });


        AudioManager myAudioMgr = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        String nativeSampleRate = myAudioMgr.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE);
        OLog.d(TAG + " hardware support samplerate: " + nativeSampleRate);
    }

    private void setPlayerListener() {
        jniffPlayer.setOnProgressListener(new JNIffPlayer.OnProgressListener() {
            @Override
            public void onProgress(final int progress) {
                OLog.d(TAG + ",from JNI, onProgress:" + progress);
                if (!isTouch) {
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            int duration = jniffPlayer.getDuration();
                            OLog.d(TAG + ", duration: " + duration);
                            if (duration != 0) {
                                if (isSeek) {
                                    isSeek = false;
                                    return;
                                }
                                binding.seekBar.setProgress(progress * 100 / duration);
                            }
                        }
                    });
                }
            }
        });

        jniffPlayer.setOnPrepareListener(new JNIffPlayer.OnPrepareListener() {
            @Override
            public void onPrepared() {
                OLog.d(TAG + ",from JNI, prepare finish, now start");
                int duration = jniffPlayer.getDuration();
                if (duration != 0) {
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            binding.seekBar.setVisibility(View.VISIBLE);
                        }
                    });
                } else {
                    OLog.d(TAG + ", play type is Live.");
                }

                jniffPlayer.start();
            }
        });

        jniffPlayer.setOnErrorListener(new JNIffPlayer.OnErrorListener() {
            @Override
            public void onError(int errorCode) {
                OLog.d(TAG + ",from JNI, Error:" + errorCode);
            }
        });
    }

    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        OLog.d(TAG + " onProgressChanged, progress:" + progress + ", fromUser:" + fromUser);
    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {
        isTouch = true;
    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {
        isSeek = true;
        isTouch = false;
        int curSeekBarProgress = binding.seekBar.getProgress();
        int duration = jniffPlayer.getDuration();
        int seekPoint = curSeekBarProgress * duration / 100;
        jniffPlayer.seekTo(seekPoint);
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onStop() {
        super.onStop();
        jniffPlayer.stop();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        jniffPlayer.release();
    }
}