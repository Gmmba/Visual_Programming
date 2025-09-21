import kotlin.math.cos
import kotlin.math.sin
import kotlin.math.PI
import kotlin.random.Random

open class Human(
    var fio: String,
    var age: Int,
    var speed: Int
) {
    protected var x: Double = 0.0
    protected var y: Double = 0.0
    protected val random = Random.Default

    open fun move() {
        val thread = Thread {
            repeat(5) { i ->
                val alpha = 2 * PI * random.nextDouble()
                x += speed * cos(alpha)
                y += speed * sin(alpha)

                log("$fio (Human) шагает в ($x, $y) [секунда ${i + 1}]")
                Thread.sleep(1000)
            }
        }
        thread.start()
    }

    @Synchronized
    protected fun log(msg: String) {
        println(msg)
    }
}

class Driver(
    fio: String,
    age: Int,
    speed: Int
) : Human(fio, age, speed) {

    private var direction: Double = 2 * PI * random.nextDouble()

    override fun move() {
        val thread = Thread {
            repeat(5) { i ->
                x += speed * cos(direction)
                y += speed * sin(direction)

                log("$fio (Driver) едет в ($x, $y) [секунда ${i + 1}]")
                Thread.sleep(1000)
            }
        }
        thread.start()
    }
}

fun main() {
    val humans = listOf(
        Human("Петров Т.И", 19, 10),
        Human("Иванов К.П", 9, 7),
        Human("Смирнов Р.Е", 29, 12),
        Human("Кузнецов Г.Н", 30, 9)
    )
    val driver = Driver("Сидоров А.В", 40, 20)

    val all = humans + driver

    all.forEach { it.move() }

    Thread.sleep(6000)
}
