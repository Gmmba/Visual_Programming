import kotlin.math.cos
import kotlin.math.sin
import kotlin.math.PI
import kotlin.random.Random

class Human(
    var fio: String,
    var age: Int,
    var speed: Int
) {
    private var x: Double = 0.0
    private var y: Double = 0.0
    private val random = Random.Default

    fun move() {
        val alpha = 2 * PI * random.nextDouble()

        // перемещаемся по направлению угла
        x += speed * cos(alpha)
        y += speed * sin(alpha)

        println("$fio переместился в ($x, $y)")
    }
}

fun main() {
    val humans = arrayOf(
        Human("Петров Т.И", 19, 10),
        Human("Иванов К.П", 9, 7),
        Human("Смирнов Р.Е", 29, 12),
        Human("Кузнецов Г.Н", 30, 9),
        Human("Волков Л.Г", 39, 16),
        Human("Зайцев В.В", 21, 11),
        Human("Козлов Ш.Л", 35, 13),
        Human("Тарасов Е.Е", 16, 17),
        Human("Титов Т.Т", 80, 6),
        Human("Куликов Л.О", 59, 8)
    )

    repeat(5) { i ->
        println("Секунда ${i + 1}")
        humans.forEach { it.move() }
        println()
        Thread.sleep(1000)
    }
}