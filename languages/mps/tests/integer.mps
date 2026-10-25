* Exercises the FREE keyword on the NAME card, an INTORG/INTEND marker
* block around integer columns, and an RHS/BOUNDS set literally named
* after its own section keyword.
NAME          MIPTEST               FREE
ROWS
 N  COST
 L  LIM1
 G  LIM2
COLUMNS
    MARKER1                 'MARKER'                 'INTORG'
    XONE      COST                 1   LIM1                 1
    XONE      LIM2                 1
    MARKER1                 'MARKER'                 'INTEND'
    YTWO      COST                 4   LIM1                 1
RHS
    RHS       LIM1                 5   LIM2                10
BOUNDS
 UP BOUNDS    XONE                 4
ENDATA
