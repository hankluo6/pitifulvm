class New_A {
    int x;
    New_A(int val, int val2, int val3) 
    {
        x = val + val2 * val3;
    }
    void print(int val)
    {
        System.out.println(x + val);
    }
}

class New_B{
    int x;
    New_B(int val) 
    {
        x = val;
    }
    void print(int val)
    {
        System.out.println(x - val);
    }
}

class NewAndInvokeVirtual {
    int a, b, c, d, e;
    public void print(int x) {
        System.out.println(123123);
    }
    public static void main(String args[]) {
        NewAndInvokeVirtual x = new NewAndInvokeVirtual();
        New_A obj1 = new New_A(5555, 88, 12);
        New_B obj2 = new New_B(3);
        x.a = 3;
        x.b = 4;
        x.c = 5;
        x.d = 6;
        x.e = 7;
        obj1.print(11);
        obj2.print(22);
        System.out.println(x.e);
        System.out.println(x.d);
        System.out.println(x.c);
        System.out.println(x.b);
        System.out.println(x.a);
        obj1.print(4555);
        obj2.print(888);
    }
}