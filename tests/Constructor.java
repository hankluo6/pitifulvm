class A {
    A(int val1, int val2) {
        x = val1;
        y = val2;
    }
    public static int x(int x) {
        return 3;
    }
    int x, y;
}

class B {
    B(int val) {
        x = val;
    }
    public static int x(int x) {
        return 5;
    }
    int x;
}

class C {
    C(int val1, int val2, int val3) {
        x = val1;
        y = val2;
        z = val3;
    }
    public static int x(int x) {
        return 1000;
    }
    int x, y, z;
}

class Constructor {

    public static void print(int x) {
        System.out.println(x);
    }

    public static void main(String[] args) {
        A a1 = new A(12, 1);
        C c1 = new C(555, 111, 222);
        B b1 = new B(8);
        B b2 = new B(9);
        //print(9999);
    }
}