package com.example.calculator

import android.content.Intent
import android.os.Bundle
import android.widget.Button
import androidx.appcompat.app.AppCompatActivity

class MainActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        val btnCalc = findViewById<Button>(R.id.btnCalc)
        val btnPlayer = findViewById<Button>(R.id.btnPlayer)
        val btnLoc = findViewById<Button>(R.id.btnLoc)
        val btnTel = findViewById<Button>(R.id.btnTel)
        val btnSoc = findViewById<Button>(R.id.btnSoc)
        val btnService = findViewById<Button>(R.id.btnService)

        btnCalc.setOnClickListener {
            val intent = Intent(this, CalculatorActivity::class.java)
            startActivity(intent)
        }
        btnPlayer.setOnClickListener {
            val intent = Intent(this, MediaPlayerActivity::class.java)
            startActivity(intent)
        }
        btnLoc.setOnClickListener {
            val intent = Intent(this, LocationActivity::class.java)
            startActivity(intent)
        }
        btnTel.setOnClickListener {
            val intent = Intent(this, TelephonyActivity::class.java)
            startActivity(intent)
        }
        btnSoc.setOnClickListener {
            val intent = Intent(this, SocketsActivity::class.java)
            startActivity(intent)
        }
        btnService.setOnClickListener {
            val intent = Intent(this, ServiceActivity::class.java)
            startActivity(intent)
        }
    }
}