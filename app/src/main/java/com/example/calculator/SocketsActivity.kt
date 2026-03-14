package com.example.calculator

import android.os.Bundle
import android.util.Log
import android.widget.Button
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import org.zeromq.SocketType
import org.zeromq.ZContext

class SocketsActivity : AppCompatActivity() {

    private val TAG = "SOCKETS_CLIENT"

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_sockets)

        findViewById<Button>(R.id.btnSend).setOnClickListener {
            val SERVER_IP = "192.168.0.16"
            val context = ZContext()
            val socket = context.createSocket(SocketType.REQ)
            socket.connect("tcp://$SERVER_IP:12345")
            socket.send("Hello from Android!", 0)
            Log.d(TAG, "Отправлено: Hello from Android!")
            val reply = socket.recv(0)
            Log.d(TAG, "Получено: $reply")
            Toast.makeText(this, "Ответ: $reply", Toast.LENGTH_SHORT).show()
            socket.close()
            context.close()
        }
    }
}