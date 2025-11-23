package com.example.calculator

import android.Manifest
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.location.Location
import android.location.LocationListener
import android.location.LocationManager
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.provider.Settings
import android.widget.TextView
import android.widget.Toast
import androidx.annotation.RequiresPermission
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import java.io.File
import java.io.FileWriter
import java.text.SimpleDateFormat
import java.util.*

class LocationActivity : AppCompatActivity(), LocationListener {

    private lateinit var locationManager: LocationManager
    private lateinit var tvLat: TextView
    private lateinit var tvLon: TextView
    private lateinit var tvAlt: TextView
    private lateinit var tvTime: TextView

    private val PERMISSION_REQUEST_CODE = 100

    private val timeHandler = Handler(Looper.getMainLooper())
    private val timeRunnable : () -> Unit = {
        updateCurrentTime()
        timeHandler.postDelayed(timeRunnable, 1000)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_location)

        tvLat = findViewById(R.id.tv_lat)
        tvLon = findViewById(R.id.tv_lon)
        tvAlt = findViewById(R.id.tv_alt)
        tvTime = findViewById(R.id.tv_time)

        locationManager = getSystemService(Context.LOCATION_SERVICE) as LocationManager
    }

    @RequiresPermission(allOf = [Manifest.permission.ACCESS_FINE_LOCATION, Manifest.permission.ACCESS_COARSE_LOCATION])
    override fun onResume() {
        super.onResume()
        timeHandler.post(timeRunnable)

        if (hasPermissions()) {
            if (isLocationEnabled()) {
                startLocation()
            } else {
                openSettings()
            }
        } else {
            requestPermissions()
        }
    }

    override fun onPause() {
        super.onPause()
        timeHandler.removeCallbacks(timeRunnable)
        locationManager.removeUpdates(this)
    }

    private fun hasPermissions() =
        ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) == PackageManager.PERMISSION_GRANTED &&
                ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_COARSE_LOCATION) == PackageManager.PERMISSION_GRANTED

    private fun requestPermissions() {
        ActivityCompat.requestPermissions(
            this,
            arrayOf(Manifest.permission.ACCESS_FINE_LOCATION, Manifest.permission.ACCESS_COARSE_LOCATION),
            PERMISSION_REQUEST_CODE
        )
    }

    private fun isLocationEnabled() =
        locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER)

    private fun openSettings() {
        Toast.makeText(this, "Включите геолокацию в настройках", Toast.LENGTH_SHORT).show()
        startActivity(Intent(Settings.ACTION_LOCATION_SOURCE_SETTINGS))
    }

    @RequiresPermission(allOf = [Manifest.permission.ACCESS_FINE_LOCATION, Manifest.permission.ACCESS_COARSE_LOCATION])
    private fun startLocation() {
        locationManager.requestLocationUpdates(LocationManager.GPS_PROVIDER, 5000, 1f, this)

        locationManager.getLastKnownLocation(LocationManager.GPS_PROVIDER)?.let { location ->
            showLocation(location)
        }
    }

    private fun updateCurrentTime() {
        val now = System.currentTimeMillis()
        val timeStr = SimpleDateFormat("dd.MM.yyyy HH:mm:ss", Locale.getDefault())
            .apply { timeZone = TimeZone.getDefault() }
            .format(Date(now))
        tvTime.text = "Время: $timeStr"
    }

    private fun showLocation(location: Location) {
        tvLat.text = "Широта: ${location.latitude}"
        tvLon.text = "Долгота: ${location.longitude}"
        tvAlt.text = "Высота: ${location.altitude} м"
    }

    private fun saveToJson(location: Location) {
        val timeStr = SimpleDateFormat("dd.MM.yyyy HH:mm:ss", Locale.getDefault())
            .apply { timeZone = TimeZone.getDefault() }
            .format(Date(location.time))

        val json = """
        {
            "latitude": ${location.latitude},
            "longitude": ${location.longitude},
            "altitude": ${location.altitude},
            "time": "$timeStr"
        }
        """.trimIndent()

        val file = File(externalCacheDir, "location.json")
        FileWriter(file, true).use { it.write(json + "\n") }
    }

    override fun onLocationChanged(location: Location) {
        showLocation(location)
        saveToJson(location)
    }

    @RequiresPermission(allOf = [Manifest.permission.ACCESS_FINE_LOCATION, Manifest.permission.ACCESS_COARSE_LOCATION])
    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == PERMISSION_REQUEST_CODE && grantResults.getOrNull(0) == PackageManager.PERMISSION_GRANTED) {
            if (isLocationEnabled()) startLocation() else openSettings()
        }
    }
}