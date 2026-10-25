package com.example.pointcuts;

public aspect PointcutCoverage {

    // call / execution / withincode -- method pattern
    pointcut callMethod() : call(public void com.example.Foo.bar(int, String));
    pointcut execMethod() : execution(* com.example..*.*(..));
    pointcut withinCodeMethod() : withincode(* com.example.Foo.bar(..));

    // call / execution -- constructor pattern
    pointcut callCtor() : call(com.example.Foo.new(..));
    pointcut execCtor() : execution(com.example.Foo.new(..));

    // initialization / preinitialization
    pointcut init() : initialization(com.example.Foo.new(..));
    pointcut preinit() : preinitialization(com.example.Foo.new(..));

    // staticinitialization, plain and annotated
    pointcut staticInit() : staticinitialization(com.example.Foo);
    pointcut staticInitAnn() : staticinitialization(@com.example.Ann *);

    // get / set
    pointcut fieldGet() : get(* com.example.Foo.count);
    pointcut fieldSet() : set(!public int com.example.Foo.count);

    // handler
    pointcut handled() : handler(Exception+);

    // adviceexecution
    pointcut adviceExec() : adviceexecution();

    // within / withincode with annotated type pattern
    pointcut withinType() : within(com.example..*);
    pointcut withinAnnType() : within(@com.example.Ann *);

    // cflow / cflowbelow
    pointcut flowOf() : cflow(callMethod());
    pointcut flowBelowOf() : cflowbelow(callMethod());

    // if
    pointcut ifTrue() : if(true);
    pointcut ifExpr() : if(1 > 0);
    pointcut ifBare() : if();

    // this / target
    pointcut thisType() : this(com.example.Foo);
    pointcut targetType() : target(com.example.Foo);

    // args
    pointcut anyArgs() : args(int, ..);
    pointcut wildArgs() : args(*, String, ..);
    pointcut typedArgs() : args(int, String);

    // @this / @target / @within / @withincode / @annotation
    pointcut annThis() : @this(com.example.Ann);
    pointcut annTarget() : @target(com.example.Ann);
    pointcut annWithin() : @within(com.example.Ann);
    pointcut annWithinCode() : @withincode(com.example.Ann);
    pointcut annAnnotated() : @annotation(com.example.Ann);

    // @args
    pointcut annArgs() : @args(com.example.Ann, *, ..);

    // reference pointcuts, bare and qualified
    pointcut refBare() : callMethod();
    pointcut refQualified() : com.example.pointcuts.PointcutCoverage.callMethod();

    // composition: !, &&, ||, parens
    pointcut composed() : callMethod() && !execMethod();
    pointcut composedOr() : callMethod() || execMethod() && withinType();
    pointcut composedParens() : (callMethod() || execMethod()) && !withinType();

    // type patterns: +, arrays, negation, &&/||
    pointcut typePatternPlus() : within(Exception+);
    pointcut typePatternArray() : within(int[]);
    pointcut typePatternNot() : within(!com.example.Foo);
    pointcut typePatternAnd() : within(com.example.Foo && com.example.Bar);
    pointcut typePatternOr() : within(com.example.Foo || com.example.Bar);

    // field/method patterns: modifiers, negated modifiers, throws
    pointcut modPattern() : call(public static * *.*(..));
    pointcut negModPattern() : call(!private * *.*(..));
    pointcut throwsPattern() : call(* *.*(..) throws java.io.IOException);

    // declaring type + dotOrDotDot
    pointcut declaringDot() : call(* com.example.Foo.bar(..));
    pointcut declaringDotDot() : call(* com.example..bar(..));

    // wildcarded member name
    pointcut wildName() : call(* get*(..));
    pointcut wildName2() : call(* *Value(..));
}
