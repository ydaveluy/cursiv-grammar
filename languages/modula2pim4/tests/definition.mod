DEFINITION MODULE Defs;

  EXPORT QUALIFIED Color, MakeColor;

  CONST
    MaxColors = 16;

  TYPE
    Color = (Red, Green, Blue);
    Handle;
    Range = [0..99];

  VAR
    Count: INTEGER;

  PROCEDURE MakeColor(n: INTEGER): Color;

END Defs.
