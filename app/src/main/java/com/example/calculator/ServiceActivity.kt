package com.example.calculator

import android.content.Intent
import android.os.Bundle
import android.widget.Button
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity

class ServiceActivity : AppCompatActivity() {

    private lateinit var tvStatus: TextView
    private lateinit var btnStart: Button
    private lateinit var btnStop: Button

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_service)

        tvStatus = findViewById(R.id.tv_status)
        btnStart = findViewById(R.id.btn_start)
        btnStop = findViewById(R.id.btn_stop)

        btnStart.setOnClickListener {
            startService(Intent(this, Service::class.java).apply {
                action = Service.ACTION_START
            })
            tvStatus.text = "Сервис запущен. Данные отправляются на сервер..."
            Toast.makeText(this, "Фоновый сервис запущен", Toast.LENGTH_SHORT).show()
        }

        btnStop.setOnClickListener {
            stopService(Intent(this, Service::class.java).apply {
                action = Service.ACTION_STOP
            })
            tvStatus.text = "Сервис остановлен"
            Toast.makeText(this, "Фоновый сервис остановлен", Toast.LENGTH_SHORT).show()
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        stopService(Intent(this, Service::class.java).apply {
            action = Service.ACTION_STOP
        })
    }
}