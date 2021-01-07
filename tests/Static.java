public class Static {
    static int x, y, z;
    public static void main(String[] args) {
        x = 3;
        y = 5;
        z = 3 * 4;
        Static_A.w = 1;
        Static_B.w = x + y / z * Static_A.w;
        System.out.println(Static.x);
        System.out.println(Static.y);
        System.out.println(Static.z);
        System.out.println(Static_A.w);
        System.out.println(Static_B.w);
    }
}

class Static_A {
    static int w;
}

class Static_B {
    static int w;
}