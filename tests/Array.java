class Array 
{
    public static void main(String args[])
    {
        int[] x = new int[1];
        int[] y = new int[2];
        int[][] z = new int[5][5];
        y[0] = 3;
        x[0] = 1;
        y[1] = 5;
        z[2][4] = 8;
        z[1][3] = z[2][4] + 5;
        System.out.println(x[0]);
        System.out.println(y[0]);
        System.out.println(y[1]);
        System.out.println(z[1][3]);
    }
}

