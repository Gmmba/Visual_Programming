package com.example.calculator

import android.os.Bundle
import android.widget.Button
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity

class MainActivity : AppCompatActivity() {

    private lateinit var tvResult: TextView
    private var input: String = ""

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        tvResult = findViewById(R.id.tvResult)

        val buttons = listOf(
            R.id.btn0, R.id.btn1, R.id.btn2, R.id.btn3, R.id.btn4,
            R.id.btn5, R.id.btn6, R.id.btn7, R.id.btn8, R.id.btn9,
            R.id.btnPlus, R.id.btnMinus, R.id.btnMul, R.id.btnDiv
        )

        // Обработка цифр и операций
        for (id in buttons) {
            findViewById<Button>(id).setOnClickListener {
                val text = (it as Button).text.toString()
                input += text
                tvResult.text = input
            }
        }

        // Очистка
        findViewById<Button>(R.id.btnClear).setOnClickListener {
            input = ""
            tvResult.text = "0"
        }

        // Равно — вычисление выражения
        findViewById<Button>(R.id.btnEqual).setOnClickListener {
            val result = calculate(input)
            tvResult.text = result
            input = result
        }
    }

    private fun calculate(expression: String): String {
        try {
            // Находим оператор и разбиваем строку
            val operator = when {
                expression.contains("+") -> "+"
                expression.contains("-") -> "-"
                expression.contains("*") -> "*"
                expression.contains("/") -> "/"
                else -> return expression
            }

            val parts = expression.split(operator)
            if (parts.size != 2) return expression

            val a = parts[0].toDoubleOrNull() ?: return "Ошибка"
            val b = parts[1].toDoubleOrNull() ?: return "Ошибка"

            val res = when (operator) {
                "+" -> a + b
                "-" -> a - b
                "*" -> a * b
                "/" -> if (b != 0.0) a / b else return "∞"
                else -> 0.0
            }
            return res.toString()
        } catch (_: Exception) {
            return "Ошибка"
        }
    }
}