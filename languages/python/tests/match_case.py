"""match/case patterns, and match/case/type/_ used as plain names."""


def describe(point):
    match point:
        case Point(x=0, y=0):
            return "origin"
        case Point(x=0, y=y):
            return f"Y={y}"
        case Point(x=x, y=0):
            return f"X={x}"
        case Point():
            return "elsewhere"
        case [Point(0, 0), Point(x, y), *rest]:
            return x, y, rest
        case {"kind": "point", **fields}:
            return fields
        case (1, 2) | (3, 4):
            return "known pair"
        case str() | bytes() as literal:
            return literal
        case _:
            return "not a point"


# Soft keywords stay ordinary identifiers outside pattern position.
match = 1
case = 2
type = 3
_ = 4


def match(case):
    return match, case, type, _


class case:
    match = 1


match(case)
case(match)
