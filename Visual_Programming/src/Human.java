import java.util.Random;

public class Human {
    private String fio;
    private int age;
    private int speed;
    private double x;
    private double y;
    private Random random = new Random();

    public Human(String fio, int age, int speed) {
        this.fio = fio;
        this.age = age;
        this.speed = speed;
        this.x = 0;
        this.y = 0;
    }

    public void setFio(String fio) {
        this.fio = fio;
    }
    public void setAge(int age) {
        this.age = age;
    }
    public void setSpeed(int speed) {
        this.speed = speed;
    }

    public String getFio() {
        return fio;
    }
    public int getAge() {
        return age;
    }
    public int getSpeed() {
        return speed;
    }

    public void move() {
        double alpha = 2 * Math.PI * random.nextDouble();

        // перемещаемся по направлению угла
        x += speed * Math.cos(alpha);
        y += speed * Math.sin(alpha);

        System.out.println(fio + " переместился в (" + x + ", " + y + ")");
    }
}
