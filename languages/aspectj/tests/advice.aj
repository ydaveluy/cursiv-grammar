package com.example.advice;

public aspect AdviceExamples {

    before() : call(* *.*(..)) {
        System.out.println("before");
    }

    before(int x) : args(x, ..) {
        System.out.println(x);
    }

    after() : call(* *.*(..)) {
        System.out.println("after");
    }

    after() returning : call(* *.*(..)) {
        System.out.println("after returning");
    }

    after() returning(Object result) : call(* *.*(..)) {
        System.out.println(result);
    }

    after() throwing : call(* *.*(..)) {
        System.out.println("after throwing");
    }

    after() throwing(Exception e) : call(* *.*(..)) {
        System.out.println(e.getMessage());
    }

    void around() : call(* *.*(..)) {
        proceed();
    }

    Object around() : call(* *.*(..)) {
        return proceed();
    }

    strictfp before() : call(* *.*(..)) {
    }

    before() throws java.io.IOException : call(* *.*(..)) {
    }
}
