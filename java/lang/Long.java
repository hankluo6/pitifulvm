package java.lang;

/* reference: http://hg.openjdk.java.net/jdk8/jdk8/jdk/file/f92ab6dbbff8/src/share/classes/java/lang/Long.java */

public class Long {
    private final long value;
    public static final int SIZE = 64;
    public static final int BYTES = 8;
    public static final long MIN_VALUE = -9223372036854775808L;
    public static final long MAX_VALUE = 9223372036854775807L;

    public Long() {
        this.value = 0;
    }

    public Long(long value) {
        this.value = value;
    }

    public static native long parseLong(String s);

    public static Long valueOf(String s) {
        return new Long(parseLong(s));
    }
    public long longValue() {
        return this.value;
    }
}
