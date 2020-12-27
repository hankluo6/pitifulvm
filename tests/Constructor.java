class A {
    A(int val1, int val2) {
        System.out.println(1);
    }
    public static int x(int x) {
        System.out.println(x);
        return 3;
    }
}

class B {
    B(int val) {
        System.out.println(3);
    }
    public static int x(int x) {
        System.out.println(x);
        return 5;
    }
}

class C {
    C(int val1, int val2, int val3) {
        System.out.println(5);
    }
    public static int x(int x) {
        System.out.println(x);
        return 1000;
    }
}

class Constructor {

    public static void print(int x) {
        System.out.println(x);
    }

    public static void main(String[] args) {
        System.out.println(A.x(5));
        System.out.println(B.x(88));
        A a1 = new A(12, 1);
        System.out.println(C.x(1205));
        C c1 = new C(555, 111, 222);
        B b1 = new B(8);
        B b2 = new B(9);
        print(9999);
    }
}
