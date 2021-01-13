public class Strings {
    public static void main(String args[])
    {
        String str1 = "Hello";
        String str2 = " Jvm ";
        String str3 = "string";
        String str4 = "La\bLa\tLa\\La\nLa\1\2\3\4\5\6Linux";
        System.out.println(str4);
        System.out.println(str1 + " constant " + str2);
        System.out.println(str3 + str1 + str2);
        System.out.println("prefix " + str4 + " postfix");
        System.out.println("1" + 2 + 5 + 3.8 + str2 + "2");
        System.out.println(str1 + str2 + str3 + str4);
    }
}
