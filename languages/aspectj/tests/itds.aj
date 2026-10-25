package com.example.itds;

public aspect InterTypeExamples {

    // inter-type method (with body)
    public void com.example.Foo.doWork() {
        System.out.println("working");
    }

    // inter-type method with return type and throws
    public String com.example.Foo.describe() throws java.io.IOException {
        return "foo";
    }

    // abstract inter-type method
    public abstract void com.example.Bar.doAbstract();

    // inter-type constructor
    public com.example.Foo.new(int x) {
        this();
    }

    // inter-type field, with and without initializer
    private int com.example.Foo.counter = 0;
    public static String com.example.Foo.name;

    // declare parents extends / implements
    declare parents : com.example.Foo extends com.example.Base;
    declare parents : com.example.Foo implements Runnable, Comparable;

    // declare warning / error
    declare warning : call(* com.example.Foo.deprecated(..)) : "do not call deprecated";
    declare error : call(* com.example.Foo.forbidden(..)) : "forbidden call";

    // declare soft
    declare soft : java.io.IOException : execution(* com.example.Foo.*(..));

    // declare precedence
    declare precedence : com.example.HighPriority, com.example.LowPriority, *;

    // declare @type / @method / @constructor / @field
    declare @type : com.example.Foo : @Deprecated;
    declare @method : * com.example.Foo.*(..) : @Deprecated;
    declare @constructor : com.example.Foo.new(..) : @Deprecated;
    declare @field : * com.example.Foo.* : @Deprecated;
}
