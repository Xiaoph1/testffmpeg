package com.example.testffmpeg

import android.Manifest
import android.content.pm.PackageManager
import android.graphics.PixelFormat
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.widget.TextView
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import androidx.lifecycle.lifecycleScope
import com.example.testffmpeg.databinding.ActivityMainBinding
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.util.Objects

class MainActivity : AppCompatActivity(), SurfaceHolder.Callback {

    //par
    private lateinit var binding: ActivityMainBinding
    private lateinit var surfaceView: SurfaceView
    private lateinit var surfaceHolder: SurfaceHolder

    //external function
    external fun stringFromJNI(): String
    external fun open(url: String, surface: Surface) // 传递 Surface 到 Native

    companion object {
        // Used to load the 'testffmpeg' library on application startup.
        init {
            System.loadLibrary("testffmpeg")
        }
    }
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        // 初始化 SurfaceView 相关（通过 binding 访问控件）
        binding.surfaceView.holder.addCallback(this)
        binding.surfaceView.holder.setFormat(PixelFormat.RGBA_8888) // 匹配 RGBA 格式

        // 动态申请存储权限
        requestPermissions()

        // Example of a call to a native method
//        binding.sampleText.text = stringFromJNI()

    }

    // 申请权限
    private fun requestPermissions() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE)
            != PackageManager.PERMISSION_GRANTED
        ) {
            ActivityCompat.requestPermissions(
                this,
                arrayOf(Manifest.permission.READ_EXTERNAL_STORAGE),
                1001
            )
        }
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        // 启动视频播放（替换为你的视频路径）
        val videoPath = "/sdcard/1.mp4"
//        open(videoPath, holder.surface)
        lifecycleScope.launch {
            try {
                // 在 IO 线程执行阻塞操作
                withContext(Dispatchers.IO) {
                    open(videoPath, holder.surface)
                }
            } catch (e: Exception) {
                e.printStackTrace()
                binding.sampleText.text = "播放失败: ${e.message}"
            }
        }
    }


    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        // Surface 尺寸或格式改变时的处理
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        // Surface 销毁时的处理
    }

    override fun onDestroy() {
        super.onDestroy()
//        stopPlay()
    }
}