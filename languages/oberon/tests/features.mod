(* Constructs the one-file corpus never reaches: sections,
   record extension, arrays, pointers, procedure types, nested procedures,
   IF/ELSIF/ELSE, CASE with ranges, WHILE with ELSIF, REPEAT, FOR BY, sets,
   IN/IS, hex literals, (* nested (* comments *) too *), type guards. *)
MODULE Features;
  IMPORT SYS := System;

  CONST
    Max = 10;
    Half = Max DIV 2;
    Hex = 0FFH;
    Ch = 41X;
    Msg = "hi";

  TYPE
    Base = RECORD
      x, y: INTEGER
    END;
    Derived* = RECORD (Base)
      z: INTEGER
    END;
    Vec = ARRAY 10 OF INTEGER;
    Matrix = ARRAY 3, 4 OF REAL;
    PDerived = POINTER TO Derived;
    Proc = PROCEDURE(a: INTEGER; VAR b: ARRAY OF INTEGER): INTEGER;

  VAR
    i, j: INTEGER;
    r: REAL;
    v: Vec;
    m: Matrix;
    d: Derived;
    p: PDerived;
    s: SET;
    ok: BOOLEAN;

  PROCEDURE Add*(a, b: INTEGER): INTEGER;
  BEGIN
    RETURN a + b
  END Add;

  PROCEDURE Classify(VAR n: INTEGER; lo, hi: INTEGER): INTEGER;
    VAR result: INTEGER;

    PROCEDURE Inner(x: INTEGER): INTEGER;
    BEGIN
      RETURN x * x
    END Inner;

  BEGIN
    IF n < lo THEN
      result := -1
    ELSIF n > hi THEN
      result := 1
    ELSE
      result := 0
    END;
    RETURN Inner(result)
  END Classify;

BEGIN
  i := 0;
  j := Half;
  r := 1.5E2;
  s := {1, 3..5, i};
  ok := (i IN s) & ~ok OR (d IS Derived);
  p := NIL;
  v[0] := Add(i, j);
  m[1, 2] := r;

  WHILE i < Max DO
    i := i + 1
  ELSIF i < 0 DO
    i := 0
  END;

  REPEAT
    i := i - 1
  UNTIL i <= 0;

  FOR i := 0 TO Max BY 2 DO
    v[i] := i
  END;

  CASE i OF
    0: ok := TRUE
  | 1..3, 5: ok := FALSE
  END;

  IF p # NIL THEN
    p^.z := 9;
    d := p^(Derived)
  END
END Features.
