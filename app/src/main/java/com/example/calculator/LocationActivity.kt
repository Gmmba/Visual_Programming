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
import android.util.Log
import android.widget.TextView
import android.widget.Toast
import androidx.annotation.RequiresPermission
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import org.zeromq.ZContext
import org.zeromq.ZMQ
import java.io.File
import java.text.SimpleDateFormat
import java.util.*
import java.util.concurrent.Executors

class LocationActivity : AppCompatActivity(), LocationListener {

    private lateinit var locationManager: LocationManager
    private lateinit var tvLat: TextView
    private lateinit var tvLon: TextView
    private lateinit var tvAlt: TextView
    private lateinit var tvTime: TextView

    private val PERMISSION_REQUEST_CODE = 100
    private val TAG = "LocationActivity"

    private val timeHandler = Handler(Looper.getMainLooper())
    private val timeRunnable = object : Runnable {
        override fun run() {
            updateCurrentTime()
            timeHandler.postDelayed(this, 1000)
        }
    }

    private val executor = Executors.newSingleThreadExecutor()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_location)

        tvLat = findViewById(R.id.tv_lat)
        tvLon = findViewById(R.id.tv_lon)
        tvAlt = findViewById(R.id.tv_alt)
        tvTime = findViewById(R.id.tv_time)

        locationManager = getSystemService(Context.LOCATION_SERVICE) as LocationManager
    }

    override fun onResume() {
        super.onResume()
        timeHandler.post(timeRunnable)

        val fineLocationGranted = ContextCompat.checkSelfPermission(
            this,
            Manifest.permission.ACCESS_FINE_LOCATION
        ) == PackageManager.PERMISSION_GRANTED

        val coarseLocationGranted = ContextCompat.checkSelfPermission(
            this,
            Manifest.permission.ACCESS_COARSE_LOCATION
        ) == PackageManager.PERMISSION_GRANTED

        if (fineLocationGranted && coarseLocationGranted) {
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

    private fun requestPermissions() {
        ActivityCompat.requestPermissions(
            this,
            arrayOf(
                Manifest.permission.ACCESS_FINE_LOCATION,
                Manifest.permission.ACCESS_COARSE_LOCATION
            ),
            PERMISSION_REQUEST_CODE
        )
    }

    private fun isLocationEnabled(): Boolean {
        return locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER)
    }

    private fun openSettings() {
        Toast.makeText(this, "Включите геолокацию", Toast.LENGTH_SHORT).show()
        val intent = Intent(Settings.ACTION_LOCATION_SOURCE_SETTINGS)
        startActivity(intent)
    }

    @RequiresPermission(allOf = [Manifest.permission.ACCESS_FINE_LOCATION, Manifest.permission.ACCESS_COARSE_LOCATION])
    private fun startLocation() {
        locationManager.requestLocationUpdates(
            LocationManager.GPS_PROVIDER,
            5000,
            1f,
            this
        )

        val lastLocation = locationManager.getLastKnownLocation(LocationManager.GPS_PROVIDER)
        if (lastLocation != null) {
            showLocation(lastLocation)
        }
    }

    private fun updateCurrentTime() {
        val currentTime = System.currentTimeMillis()
        val formatter = SimpleDateFormat("dd.MM.yyyy HH:mm:ss")
        formatter.timeZone = TimeZone.getDefault()
        val formattedTime = formatter.format(Date(currentTime))
        tvTime.text = "Время: $formattedTime"
    }

    private fun showLocation(location: Location) {
        tvLat.text = "Широта: ${location.latitude}"
        tvLon.text = "Долгота: ${location.longitude}"
        tvAlt.text = "Высота: ${location.altitude} м"
    }

    private fun saveToJson(location: Location) {
        val currentTime = System.currentTimeMillis()
        val formatter = SimpleDateFormat("dd.MM.yyyy HH:mm:ss")
        formatter.timeZone = TimeZone.getDefault()
        val formattedTime = formatter.format(Date(currentTime))

        val json = """
        {
            "latitude": ${location.latitude},
            "longitude": ${location.longitude},
            "altitude": ${location.altitude},
            "time": "$formattedTime"
        }
        """

        val file = File(externalCacheDir, "location.json")
        file.writeText(json)
    }

    private fun sendToServer(location: Location) {
        var context: ZContext? = null
        var socket: ZMQ.Socket? = null

        try {
            val SERVER_IP = "10.0.2.2"
            val SERVER_PORT = 5555
            context = ZContext()
            socket = context.createSocket(ZMQ.REQ)
            socket.setReceiveTimeOut(3000)
            socket.setSendTimeOut(3000)
            socket.connect("tcp://$SERVER_IP:$SERVER_PORT")
            val currentTime = System.currentTimeMillis()
            val formatter = SimpleDateFormat("dd.MM.yyyy HH:mm:ss")
            formatter.timeZone = TimeZone.getDefault()
            val formattedTime = formatter.format(Date(currentTime))

            val json = """
                {
                    "latitude": ${location.latitude},
                    "longitude": ${location.longitude},
                    "altitude": ${location.altitude},
                    "time": "$formattedTime"
                }
            """.trimIndent()

            socket.send(json.toByteArray(Charsets.UTF_8), 0)
            val reply = socket.recvStr(0)
            if (reply != null) {
                Log.d(TAG, "Успешно отправлено на сервер: $reply")
            } else {
                Log.w(TAG, "Отправлено, но ответ не получен")
            }
        } catch (e: Exception) {
            Log.e(TAG, "Ошибка подключения к серверу: ${e.message}")
            Log.e(TAG, "Автоматическое переподключение при следующем обновлении местоположения")
        } finally {
            socket?.close()
            context?.close()
        }
    }
    override fun onLocationChanged(location: Location) {
        showLocation(location)
        saveToJson(location)
        executor.execute {
            sendToServer(location)
        }
    }
}