package com.example.calculator
import android.Manifest
import android.content.pm.PackageManager
import android.media.AudioManager
import android.media.MediaPlayer
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.os.Handler
import android.os.Looper
import android.provider.MediaStore
import android.util.Log
import android.widget.*
import androidx.appcompat.app.AppCompatActivity
import androidx.activity.result.contract.ActivityResultContracts
import androidx.core.content.ContextCompat
import java.io.File
import java.util.Locale

class MediaPlayerActivity : AppCompatActivity() {

    private var log_tag : String = "MY_LOG_TAG"
    private var mediaPlayer: MediaPlayer? = null
    private lateinit var seekBar: SeekBar
    private lateinit var volumeBar: SeekBar
    private lateinit var trackList: ListView
    private lateinit var playPauseBtn: Button

    private var currentFilePath: String? = null
    private val handler = Handler(Looper.getMainLooper())

    private val permissionLauncher = registerForActivityResult(
        ActivityResultContracts.RequestPermission()
    ) { if (it) loadTracks() else showPermissionError() }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_media_player)

        bindViews()
        setupVolumeControl()
        setupSeekbar()
        setupPlayButton()

        val perm = if (Build.VERSION.SDK_INT >= 33)
            Manifest.permission.READ_MEDIA_AUDIO
        else
            Manifest.permission.READ_EXTERNAL_STORAGE

        if (ContextCompat.checkSelfPermission(this, perm) == PackageManager.PERMISSION_GRANTED) {
            loadTracks()
        } else {
            permissionLauncher.launch(perm)
        }
    }

    private fun bindViews() {
        seekBar = findViewById(R.id.seekBar)
        volumeBar = findViewById(R.id.volumeBar)
        trackList = findViewById(R.id.trackList)
        playPauseBtn = findViewById(R.id.playPauseBtn)
    }

    private fun setupVolumeControl() {
        val am = getSystemService(AudioManager::class.java)
        volumeBar.max = am.getStreamMaxVolume(AudioManager.STREAM_MUSIC)
        volumeBar.progress = am.getStreamVolume(AudioManager.STREAM_MUSIC)
        volumeBar.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                if (fromUser) am.setStreamVolume(AudioManager.STREAM_MUSIC, progress, 0)
            }
            override fun onStartTrackingTouch(seekBar: SeekBar?) {}
            override fun onStopTrackingTouch(seekBar: SeekBar?) {}
        })
    }

    private fun setupSeekbar() {
        seekBar.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                if (fromUser) mediaPlayer?.seekTo(progress)
            }
            override fun onStartTrackingTouch(seekBar: SeekBar?) {}
            override fun onStopTrackingTouch(seekBar: SeekBar?) {}
        })
    }

    private fun setupPlayButton() {
        playPauseBtn.setOnClickListener {
            when {
                currentFilePath == null -> Toast.makeText(this, "Выберите трек", Toast.LENGTH_SHORT).show()
                mediaPlayer?.isPlaying == true -> pausePlayback()
                else -> resumeOrStart()
            }
        }
    }

    private fun loadTracks() {
        val (names, paths) = getMp3Tracks()
        if (names.isEmpty()) {
            Toast.makeText(this, "Нет MP3-файлов", Toast.LENGTH_SHORT).show()
            return
        }

        trackList.adapter = ArrayAdapter(this, android.R.layout.simple_list_item_1, names)
        trackList.onItemClickListener = AdapterView.OnItemClickListener { parent, view, pos, id ->
            if (paths[pos] != currentFilePath) {
                stopMediaPlayer()
                currentFilePath = paths[pos]
                startPlayback()
            }
        }
    }

    private fun getMp3Tracks(): Pair<List<String>, List<String>> {
        val names = mutableListOf<String>()
        val paths = mutableListOf<String>()
        val musicPath = Environment.getExternalStorageDirectory().path + "/Music"
        Log.d(log_tag, "PATH: $musicPath")
        val directory = File(musicPath)
        val files = directory.listFiles { file ->
            file.isFile && file.name.endsWith(".mp3", true)
        }

        

        val sortedFiles = files.sortedBy {
            it.name.lowercase(Locale.getDefault())
        }
        for (file in sortedFiles) {
            names.add(file.name)
            paths.add(file.absolutePath)
        }
        return names to paths
    }

    private fun startPlayback() {
        try {
            mediaPlayer = MediaPlayer().apply {
                setDataSource(currentFilePath)
                prepare()
                start()
            }
            seekBar.max = mediaPlayer!!.duration
            updateSeekBar()
            playPauseBtn.text = "Пауза"
        } catch (e: Exception) {
            Toast.makeText(this, "Ошибка воспроизведения", Toast.LENGTH_SHORT).show()
            currentFilePath = null
        }
    }

    private fun pausePlayback() {
        mediaPlayer?.pause()
        playPauseBtn.text = "Продолжить"
    }

    private fun resumeOrStart() {
        mediaPlayer?.start()
        if (mediaPlayer?.isPlaying == true) {
            updateSeekBar()
            playPauseBtn.text = "Пауза"
        } else {
            startPlayback()
        }
    }

    private fun updateSeekBar() {
        if (mediaPlayer?.isPlaying == true) {
            seekBar.progress = mediaPlayer!!.currentPosition
            handler.postDelayed(::updateSeekBar, 1000)
        }
    }

    private fun stopMediaPlayer() {
        mediaPlayer?.apply {
            if (isPlaying) stop()
            release()
        }
        mediaPlayer = null
    }

    private fun showPermissionError() {
        Toast.makeText(this, "Нужен доступ к музыке", Toast.LENGTH_LONG).show()
    }

    override fun onPause() {
        super.onPause()
        if (mediaPlayer?.isPlaying == true) pausePlayback()
    }

    override fun onDestroy() {
        super.onDestroy()
        handler.removeCallbacksAndMessages(null)
        stopMediaPlayer()
    }
}