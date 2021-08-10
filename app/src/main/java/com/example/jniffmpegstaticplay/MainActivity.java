package com.example.jniffmpegstaticplay;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

import com.example.jniffmpegstaticplay.databinding.ActivityMainBinding;

import java.io.File;

public class MainActivity extends AppCompatActivity {

    public static String httpUrl = "https://gitee.com/null_694_3232/ffmpeg-play-kot/raw/master/video_resource/Android_12_new_design_1080p.mp4";

    private ActivityMainBinding binding;
    public StaticPlayer staticPlayer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        staticPlayer = new StaticPlayer();
        staticPlayer.setSurfaceView(binding.surfaceView);

        binding.btnPlay.setOnClickListener(new View.OnClickListener(){
            @Override
            public void onClick(View view) {

                File file = new File(Environment.getExternalStorageDirectory(),"input.mp4");
                Log.d("XXXX",file.getAbsolutePath());
                //staticPlayer.start(file.getAbsolutePath());
                staticPlayer.start(httpUrl);
            }
        });
    }

    public native String stringFromJNI();
}