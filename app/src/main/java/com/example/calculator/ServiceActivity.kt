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
import android.telephony.CellIdentityLte
import android.telephony.CellInfo
import android.telephony.CellInfoGsm
import android.telephony.CellInfoLte
import android.telephony.CellInfoNr
import android.telephony.TelephonyManager
import android.util.Log
import android.widget.TextView
import android.widget.Toast
import androidx.annotation.RequiresPermission
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import org.zeromq.ZContext
import org.zeromq.ZMQ
import java.text.SimpleDateFormat
import java.util.*
import java.util.concurrent.Executors


class ServiceActivity : AppCompatActivity(), LocationListener {

    private lateinit var locationManager: LocationManager
    private lateinit var telephonyManager: TelephonyManager
    private lateinit var tvLat: TextView
    private lateinit var tvLon: TextView
    private lateinit var tvAlt: TextView
    private lateinit var tvTime: TextView
    private lateinit var tvNetwork: TextView

    private val PERMISSION_REQUEST_CODE = 100
    private val TAG = "ServiceActivity"

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
        setContentView(R.layout.activity_service)

        tvLat = findViewById(R.id.tv_lat)
        tvLon = findViewById(R.id.tv_lon)
        tvAlt = findViewById(R.id.tv_alt)
        tvTime = findViewById(R.id.tv_time)
        tvNetwork = findViewById(R.id.tv_network)

        locationManager = getSystemService(Context.LOCATION_SERVICE) as LocationManager
        telephonyManager = getSystemService(Context.TELEPHONY_SERVICE) as TelephonyManager
    }

    override fun onResume() {
        super.onResume()
        timeHandler.post(timeRunnable)

        val permissions = arrayOf(
            Manifest.permission.ACCESS_FINE_LOCATION,
            Manifest.permission.ACCESS_COARSE_LOCATION,
            Manifest.permission.READ_PHONE_STATE
        )

        val allGranted = permissions.all {
            ContextCompat.checkSelfPermission(this, it) == PackageManager.PERMISSION_GRANTED
        }

        if (allGranted) {
            if (isLocationEnabled()) {
                startLocation()
                loadTelephonyInfo()
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
                Manifest.permission.ACCESS_COARSE_LOCATION,
                Manifest.permission.READ_PHONE_STATE
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

    @RequiresPermission(Manifest.permission.ACCESS_FINE_LOCATION)
    private fun loadTelephonyInfo() {
        try {
            val cellInfoList = telephonyManager.allCellInfo
            if (cellInfoList == null || cellInfoList.isEmpty()) {
                tvNetwork.text = "Нет данных о сетях"
                return
            }

            var output = ""
            for (i in 0 until cellInfoList.size) {
                val cellInfo = cellInfoList[i]
                when (cellInfo) {
                    is CellInfoLte -> {
                        val id = cellInfo.cellIdentity
                        output += "LTE: MCC=${id.mcc}, MNC=${id.mnc}, PCI=${id.pci}, TAC=${id.tac}\n"
                    }
                    is CellInfoGsm -> {
                        val id = cellInfo.cellIdentity
                        output += "GSM: MCC=${id.mcc}, MNC=${id.mnc}, LAC=${id.lac}\n"
                    }
                }
            }
            tvNetwork.text = output
        } catch (e: Exception) {
            tvNetwork.text = "Ошибка сети"
        }
    }

    @RequiresPermission(Manifest.permission.ACCESS_FINE_LOCATION)
    private fun getNetworkData(): MutableMap<String, Any?> {
        val data = mutableMapOf<String, Any?>()
        try {
            val cellInfoList = telephonyManager.allCellInfo
            if (cellInfoList != null) {
                for (i in 0 until cellInfoList.size) {
                    val cellInfo = cellInfoList[i]
                    when (cellInfo) {
                        is CellInfoLte -> {
                            val id = cellInfo.cellIdentity
                            val sig = cellInfo.cellSignalStrength
                            data["networkType"] = "LTE"
//                            data["lteBand"] = id.getBand()
                            data["lteCellId"] = id.getPci()
                            data["lteEarfcn"] = id.getEarfcn()
                            data["lteMcc"] = id.getMcc()
                            data["lteMnc"] = id.getMnc()
                            data["ltePci"] = id.getPci()
                            data["lteTac"] = id.getTac()
                            data["lteAsuLevel"] = sig.getAsuLevel()
                            data["lteCqi"] = sig.getCqi()
                            data["lteRsrp"] = sig.getRsrp()
                            data["lteRsrq"] = sig.getRsrq()
                            data["lteRssi"] = sig.getRssi()
                            data["lteRssnr"] = sig.getRssnr()
                            data["lteTimingAdvance"] = sig.getTimingAdvance()
                        }
                        is CellInfoGsm -> {
                            val id = cellInfo.cellIdentity
                            val sig = cellInfo.cellSignalStrength
                            data["networkType"] = "GSM"
//                            data["gsmCellId"] = id.getPci()
                            data["gsmBsic"] = id.getBsic()
                            data["gsmArfcn"] = id.getArfcn()
                            data["gsmLac"] = id.getLac()
                            data["gsmMcc"] = id.getMcc()
                            data["gsmMnc"] = id.getMnc()
                            data["gsmPsc"] = id.getPsc()
                            data["gsmDbm"] = sig.getDbm()
                            data["gsmRssi"] = sig.getRssi()
                            data["gsmTimingAdvance"] = sig.getTimingAdvance()
                        }
                        is CellInfoNr -> {
                            val id = cellInfo.cellIdentity
                            val sig = cellInfo.cellSignalStrength
                            data["networkType"] = "NR"
//                            data["nrBand"] = id.getBand()
//                            data["nrNci"] = id.getNci()
//                            data["nrPci"] = id.getPci()
//                            data["nrNrarfcn"] = id.getNrarfcn()
//                            data["nrTac"] = id.getTac()
//                            data["nrMcc"] = id.getMcc()
//                            data["nrMnc"] = id.getMnc()
//                            data["nrSsRsrp"] = sig.getSsRsrp()
//                            data["nrSsRsrq"] = sig.getSsRsrq()
//                            data["nrSsSinr"] = sig.getSsSinr()
//                            data["nrTimingAdvance"] = sig.getTimingAdvance()
                        }
                    }
                }
            }
            data["networkOperator"] = telephonyManager.networkOperator
            data["networkOperatorName"] = telephonyManager.networkOperatorName
        } catch (e: Exception) {
            Log.e(TAG, "Ошибка получения сетевых данных: ${e.message}")
        }
        return data
    }

    @RequiresPermission(Manifest.permission.ACCESS_FINE_LOCATION)
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

            val networkData = getNetworkData()

            val json = """
                {
                    "latitude": ${location.latitude},
                    "longitude": ${location.longitude},
                    "altitude": ${location.altitude},
                    "accuracy": ${location.accuracy},
                    "time": "$formattedTime",
                    "networkType": "${networkData["networkType"] ?: "Unknown"}",
                    "networkOperator": "${networkData["networkOperator"] ?: ""}",
                    "networkOperatorName": "${networkData["networkOperatorName"] ?: ""}",
                    "lteMcc": ${networkData["lteMcc"] ?: -1},
                    "lteMnc": ${networkData["lteMnc"] ?: -1},
                    "ltePci": ${networkData["ltePci"] ?: -1},
                    "lteTac": ${networkData["lteTac"] ?: -1},
                    "lteBand": ${networkData["lteBand"] ?: -1},
                    "lteEarfcn": ${networkData["lteEarfcn"] ?: -1},
                    "lteRsrp": ${networkData["lteRsrp"] ?: -1},
                    "lteRsrq": ${networkData["lteRsrq"] ?: -1},
                    "lteRssi": ${networkData["lteRssi"] ?: -1},
                    "lteTimingAdvance": ${networkData["lteTimingAdvance"] ?: -1},
                    "gsmMcc": ${networkData["gsmMcc"] ?: -1},
                    "gsmMnc": ${networkData["gsmMnc"] ?: -1},
                    "gsmLac": ${networkData["gsmLac"] ?: -1},
                    "gsmCellId": ${networkData["gsmCellId"] ?: -1},
                    "gsmRssi": ${networkData["gsmRssi"] ?: -1},
                    "gsmTimingAdvance": ${networkData["gsmTimingAdvance"] ?: -1}
                }
            """.trimIndent()

            socket.send(json.toByteArray(Charsets.UTF_8), 0)
            socket.recvStr(0)

        } catch (e: Exception) {
            Log.e(TAG, "Ошибка отправки: ${e.message}")
        } finally {
            socket?.close()
            context?.close()
        }
    }

    @RequiresPermission(Manifest.permission.ACCESS_FINE_LOCATION)
    override fun onLocationChanged(location: Location) {
        showLocation(location)
        loadTelephonyInfo()
        executor.execute {
            sendToServer(location)
        }
    }
}