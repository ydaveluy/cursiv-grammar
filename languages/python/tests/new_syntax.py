"""Constructs the corpus does not happen to use."""

try:
    pass
except* (ValueError, TypeError) as eg:
    pass

try:
    pass
except OSError, RuntimeError:
    pass

degree = f"22\N{DEGREE SIGN}C"

variadic = lambda *args, **kwargs: (args, kwargs)
mixed_lambda = lambda a, /, b, *, c=1, **rest: (a, b, c, rest)


def first[T](items: list[T]) -> T:
    return items[0]


class Box[T]:
    def __init__(self, value: T) -> None:
        self.value = value


if (n := len(str(degree))) > 0:
    pass

template = t"Hello {variadic!r:>{n}}"
