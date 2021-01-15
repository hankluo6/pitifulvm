package java.lang;

import java.io.PrintStream;

public abstract class System {
    public static final PrintStream out = new PrintStream();
    public static final PrintStream err = new PrintStream();
    public native static void gc();
}
