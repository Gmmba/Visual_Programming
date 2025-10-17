import kotlin.math.PI
import kotlin.math.cos
import kotlin.math.sin

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
