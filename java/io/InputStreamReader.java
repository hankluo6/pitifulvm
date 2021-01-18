package java.io;

public class InputStreamReader extends Reader {
    public InputStream in;
    public InputStreamReader(InputStream in) {
        this.in = in;
    }
}
