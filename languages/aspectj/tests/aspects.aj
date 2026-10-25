package com.example.aspects;

public aspect LoggingAspect {

    private int counter = 0;

    pointcut anyCall() : call(* *.*(..));

    abstract pointcut abstractPointcut(Object target);

    public pointcut namedCall(String name) : call(* *.*(..)) && args(name, ..);

    void increment() {
        counter++;
    }
}

privileged aspect InternalAspect extends LoggingAspect implements Runnable {

    pointcut internalCall() : execution(* *(..));
}

aspect SingletonAspect issingleton() {
}

aspect TargetAspect pertarget(call(* *.*(..))) {
}

aspect ThisAspect perthis(execution(* *.*(..))) {
}

aspect FlowAspect percflow(call(* *.*(..))) {
}

aspect FlowBelowAspect percflowbelow(call(* *.*(..))) {
}

aspect WithinTypeAspect pertypewithin(com.example..*) {
}

class Holder {
    static aspect NestedAspect {
        pointcut nested() : this(Holder);
    }
}
