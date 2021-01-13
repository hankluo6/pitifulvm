public class Static {
    static int x = 2, y = 5, z, w = 8;
    public static void main(String[] args) {
        x = 3;
        y = 5;
        z = 3 * 4;
        Static_B.w = x + y / z * Static_A.w;
        System.out.println(Static.w);
        System.out.println(Static.x);
        System.out.println(Static.y);
        System.out.println(Static.z);
        System.out.println(Static_A.w);
        System.out.println(Static_B.w);
    }
}

class Static_A {
    static int w = 1;
}

class Static_B {
    static int w = 2;
}