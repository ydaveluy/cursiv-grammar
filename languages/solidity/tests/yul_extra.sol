pragma solidity ^0.8.0;

// Extra inline-assembly (Yul) coverage: switch with only a default case,
// multi-variable declaration and assignment from a function call, hex and
// string literals, nested blocks, and `leave`.
contract YulExtra {
    function run(uint256 x) external pure returns (uint256 out) {
        assembly "evmasm" ("size") {
            let a, b := split(x)
            a, b := split(add(a, 1))

            switch x
            default {
                a := 0
            }

            switch x
            case 0 {
                a := 1
            }
            case 1 {
                a := 2
            }

            let h := 0x1234abcd
            let s := "a yul string"
            let hs := hex"deadbeef"

            {
                let inner := add(a, b)
                out := inner
            }

            if lt(out, 1) {
                leave
            }

            function split(v) -> p, q {
                p := div(v, 2)
                q := mod(v, 2)
            }
        }
    }
}
