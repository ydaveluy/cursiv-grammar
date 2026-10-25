% assignments, operators, quoting and the special identifier chars %

A:B:3+4;

C:-A+2*3;

D:A=B;
E:A EQ B;
F1:A<>B;
G:A<=B AND A>=B;
H:A OR B;
I:A mod 3;

J:'ABC;
K:'"a quoted string";
L:"a plain string";

M:(A+B)*2;

@ARR:5;
{X}:6;
#1:7;
A[3]:8;

FUNCTION ADD(X,Y) ,
    A:X+Y,
    LOOP
        A:A+1,
        WHEN A>10 EXIT, A EXIT
    ENDLOOP
,
ENDFUN$

FUNCTION ZERO() ,
    A:0,
ENDFUN$

% BLOCK/LOOP/WHEN only ever nest inside a function body, never at top level %
FUNCTION DEMO() ,
    BLOCK
        A:1,
        B:NOT A,
    ENDBLOCK,
ENDFUN$

ADD(1,2)$
ZERO()$
DEMO()$
ADD(A:1,B:2)$
