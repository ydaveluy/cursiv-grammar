package com.example.annotations;

import org.aspectj.lang.annotation.Aspect;

@Aspect
public class LoggingAspect {

    @Before("execution(* com.example.Service.*(..))")
    public void logBefore() {
    }

    @After("execution(* com.example.Service.*(..))")
    public void logAfter() {
    }

    @AfterReturning("execution(* com.example.Service.*(..))")
    public void logAfterReturning() {
    }

    @AfterReturning(pointcut = "execution(* com.example.Service.*(..))", returning = "result")
    public void logAfterReturningBound(Object result) {
    }

    @AfterThrowing("execution(* com.example.Service.*(..))")
    public void logAfterThrowing() {
    }

    @Around("execution(* com.example.Service.*(..))")
    public Object logAround() {
        return null;
    }

    @Pointcut("call(* com.example.Service.*(..))")
    public void servicePointcut() {
    }

    @Pointcut("")
    public void emptyPointcut() {
    }

    @DeclareError("call(* com.example.Service.deprecated(..))")
    private static final String noDeprecated = "no deprecated calls";

    @DeclareWarning("call(* com.example.Service.slow(..))")
    private static final String noSlow = "slow call";

    @DeclareParents("com.example.Foo")
    private static Runnable implementedInterface;

    @DeclareParents(value = "com.example.Foo", defaultImpl = com.example.FooImpl.class)
    private static Runnable implementedInterface2;

    @DeclareMixin(value = "com.example.Foo", interfaces = { com.example.Mixin.class })
    private static com.example.MixinImpl mixinImpl;

    @DeclarePrecedence("com.example.HighPriority, com.example.LowPriority, *")
    private static final String precedence = "";
}

@Aspect("perthis(execution(* com.example.Service.*(..)))")
class PerThisAspect {
}

@Aspect
@Deprecated
class MixedAnnotations {

    @Before("call(* *.*(..))")
    @Deprecated
    void mixed() {
    }
}
