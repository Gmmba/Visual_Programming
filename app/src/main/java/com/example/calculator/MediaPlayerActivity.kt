package com.example.calculator

import android.Manifest
import android.content.pm.PackageManager
import android.database.Cursor
import android.media.AudioManager
import android.media.MediaPlayer
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.provider.MediaStore
import android.widget.*
import androidx.appcompat.app.AppCompatActivity
import androidx.activity.result.contract.ActivityResultContracts
import androidx.core.content.ContextCompat

class MediaPlayerActivity : AppCompatActivity() {

    private lateinit var mediaPlayer: MediaPlayer
    private lateinit var seekBar: SeekBar
    private lateinit var volumeBar: SeekBar
    private lateinit var trackList: ListView
    private lateinit var playPauseBtn: Button

    private var currentFilePath: String? = null
    private val handler = Handler(Looper.getMainLooper())

    private val requestPermissionLauncher = registerForActivityResult(
        ActivityResultContracts.RequestPermission()
    ) { isGranted ->
        if (isGranted) loadTracks() else showPermissionError()
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_media_player)

        seekBar = findViewById(R.id.seekBar)
        volumeBar = findViewById(R.id.volumeBar)
        trackList = findViewById(R.id.trackList)
        playPauseBtn = findViewById(R.id.playPauseBtn)

        val am = getSystemService(AUDIO_SERVICE) as AudioManager
        volumeBar.max = am.getStreamMaxVolume(AudioManager.STREAM_MUSIC)
        volumeBar.progress = am.getStreamVolume(AudioManager.STREAM_MUSIC)
        volumeBar.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(s: SeekBar?, p: Int, fromUser: Boolean) {
                if (fromUser) am.setStreamVolume(AudioManager.STREAM_MUSIC, p, 0)
            }
            override fun onStartTrackingTouch(s: SeekBar?) {}
            override fun onStopTrackingTouch(s: SeekBar?) {}
        })

        seekBar.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(s: SeekBar?, p: Int, fromUser: Boolean) {
                if (fromUser && ::mediaPlayer.isInitialized) {
                    mediaPlayer.seekTo(p)
                }
            }
            override fun onStartTrackingTouch(s: SeekBar?) {}
            override fun onStopTrackingTouch(s: SeekBar?) {}
        })

        playPauseBtn.setOnClickListener {
            togglePlayback()
        }

        val perm = if (Build.VERSION.SDK_INT >= 33) Manifest.permission.READ_MEDIA_AUDIO
        else Manifest.permission.READ_EXTERNAL_STORAGE

        if (ContextCompat.checkSelfPermission(this, perm) == PackageManager.PERMISSION_GRANTED) {
            loadTracks()
        } else {
            requestPermissionLauncher.launch(perm)
        }
    }

    private fun loadTracks() {
        val trackNames = mutableListOf<String>()
        val trackPaths = mutableListOf<String>()

        val cursor: Cursor? = contentResolver.query(
            MediaStore.Audio.Media.EXTERNAL_CONTENT_URI,
            arrayOf(MediaStore.Audio.Media.DISPLAY_NAME, MediaStore.Audio.Media.DATA),
            null,
            null,
            MediaStore.Audio.Media.DISPLAY_NAME + " ASC"
        )

        cursor?.use {
            while (it.moveToNext()) {
                val name = it.getString(0)
                val path = it.getString(1)
                if (name.endsWith(".mp3", true)) {
                    trackNames.add(name)
                    trackPaths.add(path)
                }
            }
        }

        if (trackNames.isEmpty()) {
            Toast.makeText(this, "Нет MP3-файлов в хранилище", Toast.LENGTH_SHORT).show()
            return
        }

        trackList.adapter = ArrayAdapter(this, android.R.layout.simple_list_item_1, trackNames)
        trackList.onItemClickListener = AdapterView.OnItemClickListener { _, _, i, _ ->
            val newPath = trackPaths[i]
            if (newPath == currentFilePath && ::mediaPlayer.isInitialized && mediaPlayer.isPlaying) {
                return@OnItemClickListener
            }
            stopCurrentTrack()
            currentFilePath = newPath
            startNewTrack()
        }

        mediaPlayer = MediaPlayer()
    }

    private fun startNewTrack() {
        currentFilePath?.let { path ->
            try {
                if (::mediaPlayer.isInitialized) {
                    mediaPlayer.release()
                }
                mediaPlayer = MediaPlayer().apply {
                    setDataSource(path)
                    prepare()
                    start()
                }
                seekBar.max = mediaPlayer.duration
                updateSeekBar()
                playPauseBtn.text = "⏸ Пауза"
            } catch (e: Exception) {
                Toast.makeText(this, "Ошибка воспроизведения", Toast.LENGTH_SHORT).show()
            }
        }
    }

    private fun togglePlayback() {
        if (currentFilePath == null) {
            Toast.makeText(this, "Выберите трек", Toast.LENGTH_SHORT).show()
            return
        }

        if (::mediaPlayer.isInitialized && mediaPlayer.isPlaying) {
            mediaPlayer.pause()
            playPauseBtn.text = "▶ Продолжить"
        } else {
            if (::mediaPlayer.isInitialized) {
                mediaPlayer.start()
                updateSeekBar()
                playPauseBtn.text = "⏸ Пауза"
            } else {
                startNewTrack()
            }
        }
    }

    private fun stopCurrentTrack() {
        if (::mediaPlayer.isInitialized) {
            if (mediaPlayer.isPlaying) mediaPlayer.stop()
            mediaPlayer.release()
        }
    }

    private fun updateSeekBar() {
        if (::mediaPlayer.isInitialized && mediaPlayer.isPlaying) {
            seekBar.progress = mediaPlayer.currentPosition
            handler.postDelayed(::updateSeekBar, 1000)
        }
    }

    private fun showPermissionError() {
        Toast.makeText(this, "Разрешение необходимо для доступа к музыке", Toast.LENGTH_LONG).show()
    }

    override fun onPause() {
        super.onPause()
        if (::mediaPlayer.isInitialized && mediaPlayer.isPlaying) {
            mediaPlayer.pause()
            playPauseBtn.text = "▶ Продолжить"
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        handler.removeCallbacksAndMessages(null)
        if (::mediaPlayer.isInitialized) {
            mediaPlayer.release()
        }
    }
}