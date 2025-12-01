package com.example.calculator

import android.Manifest
import android.content.pm.PackageManager
import android.os.Bundle
import android.telephony.TelephonyManager
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat

class TelephonyActivity : AppCompatActivity() {
    private val REQUEST_CODE = 100
    private lateinit var tvInfo: TextView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_telephony)
        tvInfo = findViewById(R.id.tvInfo)
    }

    override fun onResume() {
        super.onResume()
        loadTelephonyInfo()
    }

    private fun loadTelephonyInfo() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.READ_PHONE_STATE) != PackageManager.PERMISSION_GRANTED ||
            ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(
                this,
                arrayOf(
                    Manifest.permission.READ_PHONE_STATE,
                    Manifest.permission.ACCESS_COARSE_LOCATION
                ),
                REQUEST_CODE
            )
            return
        }
        val telephonyManager = getSystemService(TELEPHONY_SERVICE) as TelephonyManager
        val cellInfoList = telephonyManager.allCellInfo
        if (cellInfoList == null) {
            tvInfo.text = "Нет данных о сетях"
            return
        }
        var output = ""
        for (i in cellInfoList) {
            output += "Тип: ${i.javaClass.simpleName}\n"
            output += "Инфо: $i\n\n"
        }
        tvInfo.text = output
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == REQUEST_CODE) {
            loadTelephonyInfo()
        }
    }
}