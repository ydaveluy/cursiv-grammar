// Covers the expression language: ternary if/?/else, every binary
// operator level (power, mul, add, range, compare, istype/hastype/as/meta,
// equality, bitwise and/xor/or, implies, coalesce), unary +/-/~/not/@/@@/
// all, postfix chains (., .?, ->, [...], #(...), invocation), `new`, a
// parenthesised sequence, literals (bool, string, real, infinity, null,
// empty ()), and a block comment used as an expression placeholder.
package Expressions {
    part def Vec {
        attribute x : Real;
        attribute y : Real;
    }

    part def Sample {
        part v : Vec;

        attribute a : Boolean = true;
        attribute b : Boolean = false;
        attribute s : String = "hi";
        attribute inf : Real = *;
        attribute none = null;
        attribute empty = ();

        constraint ternary {
            if a ? 1 else 2
        }

        constraint power {
            2 ** 3 == 8
        }

        constraint arithmetic {
            (v.x + v.y) * 2 - 1 / 2 % 2 >= 0
        }

        constraint range {
            1 .. 10
        }

        constraint typecheck {
            v istype Vec and v hastype Vec and v as Vec == v
        }

        constraint equality {
            a == b and a != b and a === b and a !== b
        }

        constraint bitwise {
            (a & b) xor (a | b) implies (a ?? b)
        }

        constraint unary {
            -v.x + +v.y == ~1 or not a
        }

        constraint metaTag {
            @Vec == @@Vec
        }

        constraint allOf {
            all Vec == all Vec
        }

        constraint chained {
            v.x
        }

        constraint safeBody {
            v.?{ true }
        }

        constraint arrowCall {
            v->isEmpty()
        }

        constraint arrowBody {
            v->select{ true }
        }

        constraint indexed {
            v[1]
        }

        constraint hashed {
            v#(1)
        }

        constraint invoked {
            v.x.isSet()
        }

        constraint constructed {
            new Vec(1, 2) == new Vec(1, 2)
        }

        constraint sequence {
            (1, 2, 3)
        }

        constraint placeholder {
            /* just a comment here */
        }
    }
}
