fun main() {
    val humans = listOf(
        Human("Петров Т.И", 19, 10),
        Human("Иванов К.П", 9, 7),
        Human("Смирнов Р.Е", 29, 12),
        Human("Кузнецов Г.Н", 30, 9)
    )
    val driver = Driver("Сидоров А.В", 40, 20)

    val all: List<Movable> = humans + driver

    all.forEach { it.move() }

    Thread.sleep(6000) // ждём, пока потоки выполнятся
}
