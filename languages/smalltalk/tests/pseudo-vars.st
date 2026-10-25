| x |
x := self class.
x isNil
    ifTrue: [^nil]
    ifFalse: [^super printString , true printString , false printString]
