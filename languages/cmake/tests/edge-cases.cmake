# A zero-length bracket argument, used as a plain value rather than a
# comment.
message([[hello world]])

# A longer bracket fence: a shorter run of `=` inside (`]==]`, length 2)
# does not close a fence opened at length 3.
message([===[
content with ]==] inside, still open
]===])

# A compound argument mixing every leaf kind.
if((A "B" [[C D]] E-F))
endif()

# Identifiers may carry digits and underscores.
_private2(A2 b_c3)

# Punctuation escaped by identity, beyond the space and semicolon the other
# sample already covers.
set(x a\$b c\(d\))

# `#` not followed by a bracket opener is a line comment even when a `[`
# appears further in, or right after the `#` without an `=`-run reaching a
# second `[`.
# a [not a bracket] comment
message(ok) # [ almost a bracket

noop()
