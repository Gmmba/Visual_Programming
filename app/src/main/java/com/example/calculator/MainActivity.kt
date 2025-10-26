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

        val btn0 = findViewById<Button>(R.id.btn0)
        val btn1 = findViewById<Button>(R.id.btn1)
        val btn2 = findViewById<Button>(R.id.btn2)
        val btn3 = findViewById<Button>(R.id.btn3)
        val btn4 = findViewById<Button>(R.id.btn4)
        val btn5 = findViewById<Button>(R.id.btn5)
        val btn6 = findViewById<Button>(R.id.btn6)
        val btn7 = findViewById<Button>(R.id.btn7)
        val btn8 = findViewById<Button>(R.id.btn8)
        val btn9 = findViewById<Button>(R.id.btn9)

        val btnPlus = findViewById<Button>(R.id.btnPlus)
        val btnMinus = findViewById<Button>(R.id.btnMinus)
        val btnMul = findViewById<Button>(R.id.btnMul)
        val btnDiv = findViewById<Button>(R.id.btnDiv)

        val btnClear = findViewById<Button>(R.id.btnClear)
        val btnEqual = findViewById<Button>(R.id.btnEqual)

        btn0.setOnClickListener { addToInput("0") }
        btn1.setOnClickListener { addToInput("1") }
        btn2.setOnClickListener { addToInput("2") }
        btn3.setOnClickListener { addToInput("3") }
        btn4.setOnClickListener { addToInput("4") }
        btn5.setOnClickListener { addToInput("5") }
        btn6.setOnClickListener { addToInput("6") }
        btn7.setOnClickListener { addToInput("7") }
        btn8.setOnClickListener { addToInput("8") }
        btn9.setOnClickListener { addToInput("9") }

        btnPlus.setOnClickListener { addToInput("+") }
        btnMinus.setOnClickListener { addToInput("-") }
        btnMul.setOnClickListener { addToInput("*") }
        btnDiv.setOnClickListener { addToInput("/") }

        btnClear.setOnClickListener {
            input = ""
            tvResult.text = "0"
        }

        btnEqual.setOnClickListener {
            val result = calculate(input)
            tvResult.text = result
            input = result
        }
    }

    private fun addToInput(value: String) {
        input += value
        tvResult.text = input
    }

    private fun calculate(expression: String): String {
        try {
            var operator = ""
            if (expression.contains("+")) {
                operator = "+"
            } else if (expression.contains("-")) {
                operator = "-"
            } else if (expression.contains("*")) {
                operator = "*"
            } else if (expression.contains("/")) {
                operator = "/"
            } else {
                return expression
            }

            val parts = expression.split(operator)

            if (parts.size != 2) {
                return "Ошибка"
            }

            val number1 = parts[0].toDoubleOrNull()
            val number2 = parts[1].toDoubleOrNull()

            if (number1 == null || number2 == null) {
                return "Ошибка"
            }

            val result = when (operator) {
                "+" -> number1 + number2
                "-" -> number1 - number2
                "*" -> number1 * number2
                "/" -> {
                    if (number2 == 0.0) {
                        return "∞"
                    } else {
                        number1 / number2
                    }
                }
                else -> 0.0
            }

            return result.toString()

        } catch (e: Exception) {
            return "Ошибка"
        }
    }
}
