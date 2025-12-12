package com.example.testffmpeg

import android.Manifest
import android.content.pm.PackageManager
import android.graphics.PixelFormat
import android.os.Build
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.View
import android.view.WindowInsets
import android.view.WindowInsetsController
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
        //隐藏导航栏
        hideSystemUIAndsupportActionBar()

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        // 初始化 SurfaceView 相关（通过 binding 访问控件）
        binding.surfaceView.holder.addCallback(this)
        binding.surfaceView.holder.setFormat(PixelFormat.RGBA_8888) // 匹配 RGBA 格式

        // 动态申请存储权限
        requestPermissions()

        // Example of a call to a native method
        binding.sampleText.text = stringFromJNI()

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

    private fun hideSystemUIAndsupportActionBar(){
        supportActionBar?.hide()
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            // Android 11+ (API 30+) - 使用新API
            window.setDecorFitsSystemWindows(false)
            window.insetsController?.let { controller ->
                // 隐藏状态栏和导航栏
                controller.hide(WindowInsets.Type.systemBars())
                // 设置手势行为：滑动边缘临时显示
                controller.systemBarsBehavior =
                    WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
            }
        } else {
            // Android 10及以下 - 使用旧API
            @Suppress("DEPRECATION")
            window.decorView.systemUiVisibility = (
                    View.SYSTEM_UI_FLAG_FULLSCREEN or
                            View.SYSTEM_UI_FLAG_HIDE_NAVIGATION or
                            View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY or
                            View.SYSTEM_UI_FLAG_LAYOUT_STABLE or
                            View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN or
                            View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
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