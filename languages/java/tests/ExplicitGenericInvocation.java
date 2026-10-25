public class ExplicitGenericInvocation {
    <T> ExplicitGenericInvocation(T seed) {
    }

    ExplicitGenericInvocation() {
        <String>this("seed");
    }

    static <T> T identity(T value) {
        return value;
    }

    static void bareCall() {
        <String>identity("bare");
    }

    static class Base {
        <T> T identity(T value) {
            return value;
        }
    }

    static class Derived extends Base {
        <T> Derived() {
            <T>super();
        }

        <T> T relay(T value) {
            return <T>super.<T>identity(value);
        }
    }
}
