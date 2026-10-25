import java.util.function.IntFunction;
import java.util.function.Supplier;

public class ArrayConstructorRef {
    static IntFunction<int[]> makeIntArray() {
        return int[]::new;
    }

    static Supplier<String[]> makeStringArray() {
        return String[]::new;
    }

    static Supplier<java.util.List<String>[]> makeGenericArray() {
        return java.util.List[]::new;
    }
}
