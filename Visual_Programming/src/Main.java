public class Main {

    public static void main(String[] args) throws InterruptedException {

        Human human1 = new Human("Петров Т.И", 19, 10);
        Human human2 = new Human("Иванов К.П", 9, 7);
        Human human3 = new Human("Смирнов Р.Е", 29, 12);
        Human human4 = new Human("Кузнецов Г.Н", 30, 9);
        Human human5 = new Human("Волков Л.Г", 39, 16);
        Human human6 = new Human("Зайцев В.В", 21, 11);
        Human human7 = new Human("Козлов Ш.Л", 35, 13);
        Human human8 = new Human("Тарасов Е.Е", 16, 17);
        Human human9 = new Human("Титов Т.Т", 80, 6);
        Human human10 = new Human("Куликов Л.О", 59, 8);

        Human[] humans = {human1, human2, human3, human4, human5, human6,
        human7, human8, human9, human10};

        for (int i = 1; i < 6; i++) {
            System.out.println("Секунда " + i);
            for (int j = 0; j < 10; j++) {
                humans[j].move();
            }
            System.out.println();
            Thread.sleep(1000);
        }

    }
}