import kotlin.math.PI
import kotlin.math.cos
import kotlin.math.sin
import kotlin.random.Random

open class Human(
    var fio: String,
    var age: Int,
    override var speed: Int
) : Movable {

    override var x: Double = 0.0
    override var y: Double = 0.0
    protected val random = Random.Default

    override fun move() {
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
