grammar Features with Base hidden(WS, COMMENT)

import "./base.langium"

interface Node {
    name: string
    count?: number = 1
    active: boolean = true
    tags: string[]
    kind: 'a' | 'b'
    parent?: @Node
    wrapped: (string | number)
}

interface Special extends Node, Other {
    extra: Date;
}

type AnyNode = Node | Special

entry Model hidden(WS):
    items+=Item*
;

Loose*:
    'loose'
;

Thing returns SomeType:
    'thing'
;

Combo:
    NamedPart & FlaggedPart
;

NamedPart:
    name=ID
;

FlaggedPart:
    flagged?='yes'?
;

Checked<imperative>:
    <imperative> 'strict' | <!imperative> 'lenient'
;

UsesCheckedByName:
    Checked<imperative=true>
;

UsesCheckedPositional:
    Checked<true>
;

Predicated:
    => 'ahead' 'thing'
;

PredicatedCall:
    -> NamedPart
;

PredicatedGroupRule:
    => ('a' 'b')
;

RefBare:
    target=[NamedPart]
;

RefColon:
    target=[NamedPart:ID]
;

RefPipe:
    target=[NamedPart|ID]
;

Choice:
    value=(NamedPart | FlaggedPart)
;

Marked:
    NamedPart {NamedPart}
;

Annotated:
    val=NamedPart {NamedPart.name=current}
;

Item:
    Combo | Marked | Annotated
;

terminal HEXDIGIT: ('0'..'9' | 'a'..'f' | 'A'..'F')+;

terminal NOTDIGIT: !('0'..'9');

terminal TODASH: 'a' -> '-';

terminal ANYCH: .;

terminal LOOKAHEAD1: (?='a') 'ab';
terminal LOOKAHEAD2: (?!'a') .;
terminal LOOKAHEAD3: (?<='a') 'b';
terminal LOOKAHEAD4: (?<!'a') 'c';

terminal fragment FRAG: 'x';

hidden terminal WS: ' '+;
hidden terminal COMMENT: '#' -> '\n';
