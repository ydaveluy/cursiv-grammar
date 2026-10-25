package com.example.apex;

import System.Type;
import System.*;
import static System.Assert.areEqual;

@interface CustomMarker {
    String value() default 'none';
}

public class Generics<T extends Comparable> {
    public T item;

    public Generics(T item) {
        this.item = item;
    }

    public Generics() {
        <T>this(null);
    }

    public <U> U convertWrapper(List<U> list) {
        return list.isEmpty() ? null : list.get(0);
    }

    public static void printAll(List<? extends Object> items) {
        for (Object o : items) {
            System.debug(o);
        }
    }

    public static void printBounded(List<? super Integer> items) {
        System.debug(items.size());
    }

    public void log(String... args) {
        for (String a : args) {
            System.debug(a);
        }
    }

    public void invokeGeneric() {
        List<Integer> nums = new List<Integer>();
        Object result = this.<Integer>convertWrapper(nums);
        Generics<String> other = new Generics<String>('x');
    }
}
