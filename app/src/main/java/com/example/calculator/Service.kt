package com.example.calculator

import android.Manifest
import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
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
import androidx.core.app.NotificationCompat
import androidx.core.content.ContextCompat
import kotlinx.coroutines.*
import org.zeromq.ZContext
import org.zeromq.ZMQ
import org.json.JSONObject
import org.json.JSONArray
import java.io.File
import java.text.SimpleDateFormat
import java.util.*

class Service : Service(), LocationListener {

    companion object {
        const val ACTION_START = "com.example.calculator.action.START"
        const val ACTION_STOP = "com.example.calculator.action.STOP"
        const val TAG = "Service"
        private const val NOTIFICATION_ID = 1001
        private const val CHANNEL_ID = "location_service_channel"
        private const val DATA_FILE_NAME = "pending_data.jsonl"
    }

    private lateinit var locationManager: LocationManager
    private lateinit var telephonyManager: TelephonyManager
    private val serviceJob = Job()
    private val serviceScope = CoroutineScope(Dispatchers.IO + serviceJob)
    private var isRunning = false
    private var isSendingPending = false   // предотвращает одновременную отправку архива

    override fun onCreate() {
        super.onCreate()
        locationManager = getSystemService(Context.LOCATION_SERVICE) as LocationManager
        telephonyManager = getSystemService(Context.TELEPHONY_SERVICE) as TelephonyManager
        createNotificationChannel()
        startForeground(NOTIFICATION_ID, createNotification())
        Log.d(TAG, "Сервис создан и переведён в foreground")
    }

    private fun createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val channel = NotificationChannel(
                CHANNEL_ID,
                "Location and Network Service",
                NotificationManager.IMPORTANCE_LOW
            ).apply {
                description = "Сбор данных о местоположении и сети для отправки на сервер"
            }
            val notificationManager = getSystemService(NotificationManager::class.java)
            notificationManager.createNotificationChannel(channel)
        }
    }

    private fun createNotification(): Notification {
        val notificationIntent = Intent(this, ServiceActivity::class.java)
        val pendingIntent = PendingIntent.getActivity(
            this, 0, notificationIntent,
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) PendingIntent.FLAG_IMMUTABLE else 0
        )
        return NotificationCompat.Builder(this, CHANNEL_ID)
            .setContentTitle("Мониторинг местоположения")
            .setContentText("Данные отправляются на сервер...")
            .setSmallIcon(android.R.drawable.ic_menu_mylocation)
            .setContentIntent(pendingIntent)
            .build()
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        when (intent?.action) {
            ACTION_START -> {
                if (!isRunning) {
                    isRunning = true
                    startDataCollection()
                    // При старте пробуем отправить всё, что накопилось
                    serviceScope.launch { sendPendingData() }
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
    private suspend fun collectAndSendData(location: Location) {
        try {
            val networkData = getNetworkData()
            val currentTime = System.currentTimeMillis()
            val formatter = SimpleDateFormat("dd.MM.yyyy HH:mm:ss")
            formatter.timeZone = TimeZone.getDefault()
            val formattedTime = formatter.format(Date(currentTime))

            val json = buildJson(location, networkData, formattedTime)
            val success = sendJsonToServer(json.toString())

            if (success) {
                Log.d(TAG, "Данные отправлены: lat=${location.latitude}, lon=${location.longitude}")
                // После успешной отправки текущей точки пробуем отправить архив
                sendPendingData()
            } else {
                // Сохраняем в файл для последующей отправки
                saveDataToFile(json.toString())
                Log.d(TAG, "Данные сохранены в файл (сервер недоступен)")
            }
        } catch (e: Exception) {
            Log.e(TAG, "Ошибка сбора/отправки: ${e.message}", e)
            // При ошибке тоже сохраняем, чтобы не потерять
            try {
                val networkData = getNetworkData()
                val formatter = SimpleDateFormat("dd.MM.yyyy HH:mm:ss")
                val json = buildJson(location, networkData, formatter.format(Date()))
                saveDataToFile(json.toString())
            } catch (ex: Exception) {
                Log.e(TAG, "Не удалось даже сохранить в файл: ${ex.message}")
            }
        }
    }

    private fun buildJson(location: Location, networkData: Map<String, Any?>, time: String): JSONObject {
        return JSONObject().apply {
            put("latitude", location.latitude)
            put("longitude", location.longitude)
            put("altitude", location.altitude)
            put("accuracy", location.accuracy.toDouble())
            put("time", time)
            put("networkType", networkData["networkType"]?.toString() ?: "Unknown")
            put("networkOperator", networkData["networkOperator"]?.toString() ?: "")
            put("networkOperatorName", networkData["networkOperatorName"]?.toString() ?: "")

            // LTE
            put("lteCellId", getLongValue(networkData["lteCellId"]))
            put("lteEarfcn", getIntValue(networkData["lteEarfcn"]))
            put("lteMcc", getIntValue(networkData["lteMcc"]))
            put("lteMnc", getIntValue(networkData["lteMnc"]))
            put("ltePci", getIntValue(networkData["ltePci"]))
            put("lteTac", getIntValue(networkData["lteTac"]))
            put("lteAsuLevel", getIntValue(networkData["lteAsuLevel"]))
            put("lteCqi", getIntValue(networkData["lteCqi"]))
            put("lteRsrp", getIntValue(networkData["lteRsrp"]))
            put("lteRsrq", getIntValue(networkData["lteRsrq"]))
            put("lteRssi", getIntValue(networkData["lteRssi"]))
            put("lteRssnr", getIntValue(networkData["lteRssnr"]))
            put("lteTimingAdvance", getIntValue(networkData["lteTimingAdvance"]))

            // GSM
            put("gsmCellId", getIntValue(networkData["gsmCellId"]))
            put("gsmBsic", getIntValue(networkData["gsmBsic"]))
            put("gsmArfcn", getIntValue(networkData["gsmArfcn"]))
            put("gsmLac", getIntValue(networkData["gsmLac"]))
            put("gsmMcc", getIntValue(networkData["gsmMcc"]))
            put("gsmMnc", getIntValue(networkData["gsmMnc"]))
            put("gsmPsc", getIntValue(networkData["gsmPsc"]))
            put("gsmDbm", getIntValue(networkData["gsmDbm"]))
            put("gsmTimingAdvance", getIntValue(networkData["gsmTimingAdvance"]))

            // NR
            put("nrBand", getIntValue(networkData["nrBand"]))
            put("nrNci", getLongValue(networkData["nrNci"]))
            put("nrPci", getIntValue(networkData["nrPci"]))
            put("nrNrarfcn", getIntValue(networkData["nrNrarfcn"]))
            put("nrTac", getIntValue(networkData["nrTac"]))
            put("nrMcc", getIntValue(networkData["nrMcc"]))
            put("nrMnc", getIntValue(networkData["nrMnc"]))
            put("nrSsRsrp", getIntValue(networkData["nrSsRsrp"]))
            put("nrSsRsrq", getIntValue(networkData["nrSsRsrq"]))
            put("nrSsSinr", getIntValue(networkData["nrSsSinr"]))
            put("nrTimingAdvance", getIntValue(networkData["nrTimingAdvance"]))
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
                            data["nrMcc"] = id.mccString?.toIntOrNull() ?: 0
                            data["nrMnc"] = id.mncString?.toIntOrNull() ?: 0
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

    // ------------------- Работа с файлом -------------------
    private fun getDataFile(): File = File(filesDir, DATA_FILE_NAME)

    private fun saveDataToFile(jsonLine: String) {
        try {
            getDataFile().appendText("$jsonLine\n")
        } catch (e: Exception) {
            Log.e(TAG, "Ошибка записи в файл: ${e.message}")
        }
    }

    private fun loadAllData(): List<String> {
        val file = getDataFile()
        if (!file.exists()) return emptyList()
        return try {
            file.readLines().filter { it.isNotBlank() }
        } catch (e: Exception) {
            Log.e(TAG, "Ошибка чтения файла: ${e.message}")
            emptyList()
        }
    }

    private fun clearDataFile() {
        try {
            getDataFile().delete()
        } catch (e: Exception) {
            Log.e(TAG, "Ошибка удаления файла: ${e.message}")
        }
    }

    private suspend fun sendPendingData() {
        if (isSendingPending) return
        isSendingPending = true
        try {
            val pendingList = loadAllData()
            if (pendingList.isEmpty()) return

            Log.d(TAG, "Найдено ${pendingList.size} отложенных записей, отправляем...")
            // Отправляем одним массивом для экономии запросов
            val success = sendBatchToServer(pendingList)
            if (success) {
                clearDataFile()
                Log.d(TAG, "Отложенные данные успешно отправлены, файл очищен")
            } else {
                Log.w(TAG, "Не удалось отправить отложенные данные, оставляем в файле")
            }
        } finally {
            isSendingPending = false
        }
    }

    private suspend fun sendBatchToServer(jsonLines: List<String>): Boolean {
        val jsonArray = JSONArray()
        for (line in jsonLines) {
            try {
                jsonArray.put(JSONObject(line))
            } catch (e: Exception) {
                Log.e(TAG, "Ошибка парсинга сохранённой строки: $line", e)
            }
        }
        if (jsonArray.length() == 0) return true
        return sendJsonToServer(jsonArray.toString())
    }

    // ------------------- Отправка через ZMQ -------------------
    private suspend fun sendJsonToServer(jsonString: String): Boolean {
        return withContext(Dispatchers.IO) {
            var context: ZContext? = null
            var socket: ZMQ.Socket? = null
            try {
                val SERVER_IP = "192.168.43.34"
                val SERVER_PORT = 5555

                context = ZContext()
                socket = context.createSocket(ZMQ.REQ)
                socket.receiveTimeOut = 3000   // 3 секунды
                socket.sendTimeOut = 3000
                socket.connect("tcp://$SERVER_IP:$SERVER_PORT")

                socket.send(jsonString.toByteArray(Charsets.UTF_8))
                val reply = socket.recvStr()
                reply != null && reply == "ACK"
            } catch (e: Exception) {
                Log.e(TAG, "Ошибка отправки: ${e.message}")
                false
            } finally {
                socket?.close()
                context?.close()
            }
        }
    }

    // Вспомогательные преобразователи
    private fun getIntValue(value: Any?): Int = when (value) {
        is Int -> value
        is Long -> value.toInt()
        is String -> value.toIntOrNull() ?: 0
        else -> 0
    }

    private fun getLongValue(value: Any?): Long = when (value) {
        is Long -> value
        is Int -> value.toLong()
        is String -> value.toLongOrNull() ?: 0L
        else -> 0L
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