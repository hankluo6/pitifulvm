package java.lang;

import java.io.PrintStream;
import java.io.InputStream;

public abstract class System {
    public static final InputStream in = new InputStream();
    public static final PrintStream out = new PrintStream();
    public static final PrintStream err = new PrintStream();
    public native static void gc();
    public native static long currentTimeMillis();
}
