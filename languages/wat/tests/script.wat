(module $m
  (func (export "add") (param i32 i32) (result i32)
    local.get 0
    local.get 1
    i32.add)
  (func (export "boom")
    unreachable)
  (func (export "nan") (result f32) f32.const nan)
  (global (export "g") i32 (i32.const 5)))
(register "tested" $m)
(assert_return (invoke $m "add" (i32.const 1) (i32.const 2)) (i32.const 3))
(assert_trap (invoke $m "boom") "unreachable")
(assert_return (get $m "g") (i32.const 5))
(assert_return (invoke $m "nan") (f32.const nan:canonical))
(assert_return (invoke $m "nan") (f32.const nan:arithmetic))
(assert_exhaustion (invoke $m "add" (i32.const 1) (i32.const 1)) "call stack exhausted")
(assert_malformed
  (module quote "(module (func $f (result i32) (i32.const)))")
  "unexpected token")
(assert_invalid (module binary "\00\61\73\6d") "unexpected end")
(assert_unlinkable (module binary "\00\61\73\6d") "unknown import")
