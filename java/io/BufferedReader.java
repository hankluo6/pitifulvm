package java.io;

public class BufferedReader extends Reader {
    public Reader in;
    public BufferedReader(Reader reader) {
        this.in = in;
    }
    public native String readLine();
}
