package com.example.calculator

import android.graphics.Color
import android.os.Bundle
import android.widget.Button
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import kotlin.random.Random

class CalculatorActivity : AppCompatActivity() {

    private lateinit var tvResult: TextView
    private var input: String = ""

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_calculator)

        tvResult = findViewById(R.id.tvResult)

        val Buttons = listOf(
            R.id.btn0 to "0",
            R.id.btn1 to "1",
            R.id.btn2 to "2",
            R.id.btn3 to "3",
            R.id.btn4 to "4",
            R.id.btn5 to "5",
            R.id.btn6 to "6",
            R.id.btn7 to "7",
            R.id.btn8 to "8",
            R.id.btn9 to "9",
            R.id.btnPlus to "+",
            R.id.btnMinus to "-",
            R.id.btnMul to "*",
            R.id.btnDiv to "/"
        )

        for ((buttonId, value) in Buttons) {
            val button = findViewById<Button>(buttonId)
            button.setOnClickListener {
                addToInput(value)
                button.setBackgroundColor(getRandomColor())

            }
        }

        val btnClear = findViewById<Button>(R.id.btnClear)
        val btnEqual = findViewById<Button>(R.id.btnEqual)

        btnClear.setOnClickListener {
            btnClear.setBackgroundColor(getRandomColor())
            input = ""
            tvResult.text = "0"
        }

        btnEqual.setOnClickListener {
            btnEqual.setBackgroundColor(getRandomColor())
            val result = calculate(input)
            tvResult.text = result
            input = result
        }
    }

    private fun getRandomColor(): Int {
        val red = Random.nextInt(0, 255)
        val green = Random.nextInt(0, 255)
        val blue = Random.nextInt(0, 255)
        return Color.rgb(red, green, blue)
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