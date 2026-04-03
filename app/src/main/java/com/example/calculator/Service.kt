package com.example.calculator

import android.Manifest
import android.app.Service
import android.content.Context
import android.content.Intent
import android.location.Location
import android.location.LocationListener
import android.location.LocationManager
import android.os.IBinder
import android.telephony.*
import android.content.pm.PackageManager
import android.os.Build
import android.util.Log
import androidx.annotation.RequiresPermission
import androidx.core.content.ContextCompat
import kotlinx.coroutines.*
import org.zeromq.ZContext
import org.zeromq.ZMQ
import java.text.SimpleDateFormat
import java.util.*

class Service : Service(), LocationListener {

    companion object {
        const val ACTION_START = "com.example.calculator.action.START"
        const val ACTION_STOP = "com.example.calculator.action.STOP"
        const val TAG = "Service"
    }

    private lateinit var locationManager: LocationManager
    private lateinit var telephonyManager: TelephonyManager
    private val serviceJob = Job()
    private val serviceScope = CoroutineScope(Dispatchers.IO + serviceJob)
    private var isRunning = false

    override fun onCreate() {
        super.onCreate()
        locationManager = getSystemService(Context.LOCATION_SERVICE) as LocationManager
        telephonyManager = getSystemService(Context.TELEPHONY_SERVICE) as TelephonyManager
        Log.d(TAG, "Сервис создан")
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        when (intent?.action) {
            ACTION_START -> {
                if (!isRunning) {
                    isRunning = true
                    startDataCollection()
                    Log.d(TAG, "Сбор данных запущен")
                }
            }
            ACTION_STOP -> {
                stopSelf()
                return START_NOT_STICKY
            }
        }
        return START_STICKY
    }

    private fun startDataCollection() {
        try {
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) ==
                PackageManager.PERMISSION_GRANTED) {
                locationManager.requestLocationUpdates(
                    LocationManager.GPS_PROVIDER,
                    5000,
                    1f,
                    this
                )
            } else {
                Log.e(TAG, "Нет разрешения на местоположение")
                stopSelf()
            }
        } catch (e: SecurityException) {
            Log.e(TAG, "Ошибка безопасности: ${e.message}")
            stopSelf()
        }
    }

    @RequiresPermission(Manifest.permission.ACCESS_FINE_LOCATION)
    override fun onLocationChanged(location: Location) {
        if (!isRunning) return
        serviceScope.launch {
            collectAndSendData(location)
        }
    }

    @RequiresPermission(Manifest.permission.ACCESS_FINE_LOCATION)
    private fun collectAndSendData(location: Location) {
        try {
            val networkData = getNetworkData()
            val currentTime = System.currentTimeMillis()
            val formatter = SimpleDateFormat("dd.MM.yyyy HH:mm:ss")
            formatter.timeZone = TimeZone.getDefault()
            val formattedTime = formatter.format(Date(currentTime))

            sendToServer(location, networkData as MutableMap<String, Any>, formattedTime)
            Log.d(TAG, "Данные отправлены: lat=${location.latitude}, lon=${location.longitude}, net=${networkData["networkType"]}")
        } catch (e: Exception) {
            Log.e(TAG, "Ошибка сбора данных: ${e.message}", e)
        }
    }

    @RequiresPermission(Manifest.permission.ACCESS_FINE_LOCATION)
    private fun getNetworkData(): Map<String, Any?> {
        val data = mutableMapOf<String, Any?>()
        data["networkType"] = "Unknown"

        try {
            val cellInfoList = telephonyManager.allCellInfo
            if (cellInfoList != null && cellInfoList.isNotEmpty()) {
                for (cellInfo in cellInfoList) {
                    when (cellInfo) {
                        is CellInfoLte -> {
                            val id = cellInfo.cellIdentity
                            val sig = cellInfo.cellSignalStrength
                            data["networkType"] = "LTE"
                            data["lteCellId"] = if (id.ci != Int.MAX_VALUE) id.ci.toInt() else 0
                            data["lteEarfcn"] = if (id.earfcn != Int.MAX_VALUE) id.earfcn else 0
                            data["lteMcc"] = if (id.mcc != Int.MAX_VALUE) id.mcc else 0
                            data["lteMnc"] = if (id.mnc != Int.MAX_VALUE) id.mnc else 0
                            data["ltePci"] = if (id.pci != Int.MAX_VALUE) id.pci else 0
                            data["lteTac"] = if (id.tac != Int.MAX_VALUE) id.tac else 0
                            data["lteAsuLevel"] = if (sig.asuLevel != Int.MAX_VALUE) sig.asuLevel else 0
                            data["lteCqi"] = if (sig.cqi != Int.MAX_VALUE) sig.cqi else 0
                            data["lteRsrp"] = if (sig.rsrp != Int.MAX_VALUE) sig.rsrp else 0
                            data["lteRsrq"] = if (sig.rsrq != Int.MAX_VALUE) sig.rsrq else 0
                            data["lteRssi"] = if (sig.rssi != Int.MAX_VALUE) sig.rssi else 0
                            data["lteRssnr"] = if (sig.rssnr != Int.MAX_VALUE) sig.rssnr else 0
                            data["lteTimingAdvance"] = if (sig.timingAdvance != Int.MAX_VALUE) sig.timingAdvance else 0
                            break
                        }
                        is CellInfoGsm -> {
                            val id = cellInfo.cellIdentity
                            val sig = cellInfo.cellSignalStrength
                            data["networkType"] = "GSM"
                            data["gsmCellId"] = if (id.cid != Int.MAX_VALUE) id.cid else 0
                            data["gsmBsic"] = if (id.bsic != Int.MAX_VALUE) id.bsic else 0
                            data["gsmArfcn"] = if (id.arfcn != Int.MAX_VALUE) id.arfcn else 0
                            data["gsmLac"] = if (id.lac != Int.MAX_VALUE) id.lac else 0
                            data["gsmMcc"] = if (id.mcc != Int.MAX_VALUE) id.mcc else 0
                            data["gsmMnc"] = if (id.mnc != Int.MAX_VALUE) id.mnc else 0
                            data["gsmPsc"] = if (id.psc != Int.MAX_VALUE) id.psc else 0
                            data["gsmDbm"] = if (sig.dbm != Int.MAX_VALUE) sig.dbm else 0
                            data["gsmTimingAdvance"] = if (sig.timingAdvance != Int.MAX_VALUE) sig.timingAdvance else 0
                            break
                        }
                        is CellInfoNr -> {
                            val id = cellInfo.cellIdentity as CellIdentityNr
                            val sig = cellInfo.cellSignalStrength as CellSignalStrengthNr
                            data["networkType"] = "NR"
                            data["nrBand"] = if (Build.VERSION.SDK_INT >= 30 && id.bands?.isNotEmpty() == true) id.bands!![0] else 0
                            data["nrNci"] = if (id.nci != Long.MAX_VALUE) id.nci else 0
                            data["nrPci"] = if (id.pci != Int.MAX_VALUE) id.pci else 0
                            data["nrNrarfcn"] = if (id.nrarfcn != Int.MAX_VALUE) id.nrarfcn else 0
                            data["nrTac"] = if (id.tac != Int.MAX_VALUE) id.tac else 0
                            data["nrMcc"] = id.mccString ?: 0
                            data["nrMnc"] = id.mncString ?: 0
                            data["nrSsRsrp"] = if (sig.ssRsrp != Int.MAX_VALUE) sig.ssRsrp else 0
                            data["nrSsRsrq"] = if (sig.ssRsrq != Int.MAX_VALUE) sig.ssRsrq else 0
                            data["nrSsSinr"] = if (sig.ssSinr != Int.MAX_VALUE) sig.ssSinr else 0
                            break
                        }
                    }
                }
            }
            data["networkOperator"] = telephonyManager.networkOperator ?: ""
            data["networkOperatorName"] = telephonyManager.networkOperatorName ?: ""
        } catch (e: Exception) {
            Log.e(TAG, "Ошибка получения сетевых данных: ${e.message}", e)
        }
        return data
    }

    private fun sendToServer(location: Location, networkData: MutableMap<String, Any>, time: String) {
        var context: ZContext? = null
        var socket: ZMQ.Socket? = null

        try {
            val SERVER_IP = "192.168.43.34"
            val SERVER_PORT = 5555

            context = ZContext()
            socket = context.createSocket(ZMQ.REQ)
            socket.receiveTimeOut = 5000
            socket.sendTimeOut = 5000
            socket.connect("tcp://$SERVER_IP:$SERVER_PORT")

            val jsonObject = org.json.JSONObject()

            jsonObject.put("latitude", location.latitude)
            jsonObject.put("longitude", location.longitude)
            jsonObject.put("altitude", location.altitude)
            jsonObject.put("accuracy", location.accuracy.toDouble())
            jsonObject.put("time", time)
            jsonObject.put("networkType", networkData["networkType"]?.toString() ?: "Unknown")
            jsonObject.put("networkOperator", networkData["networkOperator"]?.toString() ?: "")
            jsonObject.put("networkOperatorName", networkData["networkOperatorName"]?.toString() ?: "")
            jsonObject.put("lteCellId", getLongValue(networkData["lteCellId"]))
            jsonObject.put("lteEarfcn", getIntValue(networkData["lteEarfcn"]))
            jsonObject.put("lteMcc", getIntValue(networkData["lteMcc"]))
            jsonObject.put("lteMnc", getIntValue(networkData["lteMnc"]))
            jsonObject.put("ltePci", getIntValue(networkData["ltePci"]))
            jsonObject.put("lteTac", getIntValue(networkData["lteTac"]))
            jsonObject.put("lteAsuLevel", getIntValue(networkData["lteAsuLevel"]))
            jsonObject.put("lteCqi", getIntValue(networkData["lteCqi"]))
            jsonObject.put("lteRsrp", getIntValue(networkData["lteRsrp"]))
            jsonObject.put("lteRsrq", getIntValue(networkData["lteRsrq"]))
            jsonObject.put("lteRssi", getIntValue(networkData["lteRssi"]))
            jsonObject.put("lteRssnr", getIntValue(networkData["lteRssnr"]))
            jsonObject.put("lteTimingAdvance", getIntValue(networkData["lteTimingAdvance"]))
            jsonObject.put("gsmCellId", getIntValue(networkData["gsmCellId"]))
            jsonObject.put("gsmBsic", getIntValue(networkData["gsmBsic"]))
            jsonObject.put("gsmArfcn", getIntValue(networkData["gsmArfcn"]))
            jsonObject.put("gsmLac", getIntValue(networkData["gsmLac"]))
            jsonObject.put("gsmMcc", getIntValue(networkData["gsmMcc"]))
            jsonObject.put("gsmMnc", getIntValue(networkData["gsmMnc"]))
            jsonObject.put("gsmPsc", getIntValue(networkData["gsmPsc"]))
            jsonObject.put("gsmDbm", getIntValue(networkData["gsmDbm"]))
            jsonObject.put("gsmTimingAdvance", getIntValue(networkData["gsmTimingAdvance"]))
            jsonObject.put("nrBand", getIntValue(networkData["nrBand"]))
            jsonObject.put("nrNci", getLongValue(networkData["nrNci"]))
            jsonObject.put("nrPci", getIntValue(networkData["nrPci"]))
            jsonObject.put("nrNrarfcn", getIntValue(networkData["nrNrarfcn"]))
            jsonObject.put("nrTac", getIntValue(networkData["nrTac"]))
            jsonObject.put("nrMcc", getIntValue(networkData["nrMcc"]))
            jsonObject.put("nrMnc", getIntValue(networkData["nrMnc"]))
            jsonObject.put("nrSsRsrp", getIntValue(networkData["nrSsRsrp"]))
            jsonObject.put("nrSsRsrq", getIntValue(networkData["nrSsRsrq"]))
            jsonObject.put("nrSsSinr", getIntValue(networkData["nrSsSinr"]))
            jsonObject.put("nrTimingAdvance", getIntValue(networkData["nrTimingAdvance"]))

            val jsonString = jsonObject.toString()

            Log.d(TAG, "Отправка JSON: $jsonString")

            socket.send(jsonString.toByteArray(Charsets.UTF_8))
            val reply = socket.recvStr()
            Log.d(TAG, "Ответ сервера: $reply")

        } catch (e: Exception) {
            Log.e(TAG, "Ошибка отправки: ${e.message}", e)
        } finally {
            socket?.close()
            context?.close()
        }
    }

    private fun getIntValue(value: Any?): Int {
        return when (value) {
            is Int -> value
            is Long -> value.toInt()
            is String -> value.toIntOrNull() ?: 0
            else -> 0
        }
    }

    private fun getLongValue(value: Any?): Long {
        return when (value) {
            is Long -> value
            is Int -> value.toLong()
            is String -> value.toLongOrNull() ?: 0L
            else -> 0L
        }
    }

    private fun getDoubleValue(value: Any?): Double {
        return when (value) {
            is Double -> value
            is Float -> value.toDouble()
            is Int -> value.toDouble()
            is String -> value.toDoubleOrNull() ?: 0.0
            else -> 0.0
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        isRunning = false
        try {
            locationManager.removeUpdates(this)
        } catch (e: Exception) {
            Log.e(TAG, "Ошибка остановки обновлений: ${e.message}")
        }
        serviceJob.cancel()
        Log.d(TAG, "Сервис остановлен")
    }

    override fun onBind(intent: Intent?): IBinder? = null
}