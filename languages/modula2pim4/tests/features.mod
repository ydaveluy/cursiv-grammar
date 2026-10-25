MODULE Features [3];

FROM SYSTEM IMPORT ADR;
IMPORT InOut;

CONST
  Base   = 16;
  Hex    = 0FFH;
  Oct    = 17B;
  Neg    = -Base;
  Greet  = "hi";
  Path   = "a/b/c";
  Quoted = 'she said "hi"';

TYPE
  Color   = (Red, Green, Blue);
  Digit   = [0..9];
  Colors  = SET OF Color;
  Vector  = ARRAY [0..9], [0..9] OF INTEGER;
  Node    = POINTER TO NodeRec;
  NodeRec = RECORD
    value: INTEGER;
    next: Node;
  END;
  Shape = RECORD
    CASE kind: INTEGER OF
      0: radius: REAL;
    | 1, 2: width: REAL; height: REAL;
    ELSE
      label: ARRAY [0..19] OF CHAR;
    END;
  END;
  Op = PROCEDURE(INTEGER, VAR INTEGER): BOOLEAN;

VAR
  count: INTEGER;
  total, ratio: REAL;
  colors: Colors;
  head: Node;
  matrix: Vector;
  shape: Shape;
  callback: Op;
  found: BOOLEAN;

MODULE Inner;
  EXPORT QUALIFIED Ping;
  VAR pings: INTEGER;
  PROCEDURE Ping;
  BEGIN
    pings := pings + 1
  END Ping;
BEGIN
  pings := 0
END Inner;

PROCEDURE Helper(x: INTEGER; VAR y: INTEGER; z: ARRAY OF INTEGER): BOOLEAN;
  VAR i: INTEGER;

  PROCEDURE Half(n: INTEGER): INTEGER;
  BEGIN
    RETURN n DIV 2
  END Half;

  BEGIN
    i := Half(x);
    y := y + i;
    RETURN x > y
  END Helper;

BEGIN
  count := 0;
  total := 0.0;
  ratio := 10.0 / 3.0;
  found := FALSE;
  matrix[0, 0] := 1;
  NEW(head);
  head^.value := 5;
  head^.next := NIL;
  WITH head^ DO
    value := value + 1
  END;
  colors := Colors{Red, Blue};
  IF Red IN colors THEN
    InOut.WriteString(Greet)
  ELSIF NOT found THEN
    InOut.WriteString(Path)
  ELSE
    InOut.WriteString(Quoted)
  END;
  CASE count OF
    0: InOut.WriteString("zero")
  | 1, 2: InOut.WriteString("small")
  | 3..9: InOut.WriteString("digit")
  ELSE
    InOut.WriteString("big")
  END;
  WHILE count < 5 DO
    count := count + 1
  END;
  REPEAT
    count := count - 1
  UNTIL count = 0;
  LOOP
    count := count + 1;
    IF count = 3 THEN EXIT END
  END;
  FOR count := 0 TO 9 BY 2 DO
    total := total + count
  END;
  found := Helper(count, count, matrix[0])
END Features.
