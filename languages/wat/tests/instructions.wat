;; A line comment.
(; a block comment (with text after the open) ;)
(module
  (type $binop (func (param i32 i32) (result i32)))
  (import "env" "log" (func $log (param i32)))
  (table 2 10 funcref)
  (memory (export "mem") 1 4)
  (global $g (mut i32) (i32.const 0))
  (global $ro i32 (i32.const 7))
  (elem (i32.const 0) $log $log)
  (data (i32.const 0) "hi")
  (start $init)

  (func $init
    (local $i i32)
    local.get $i
    i32.const 1
    i32.add
    local.set $i
    global.get $g
    local.get $i
    i32.add
    global.set $g)

  (func $add (export "add") (type $binop)
    local.get 0
    local.get 1
    i32.add)

  (func $loopy (param $n i32) (result i32)
    (local $acc i32)
    (block $done
      (loop $top
        local.get $n
        i32.eqz
        br_if $done
        local.get $acc
        local.get $n
        i32.add
        local.set $acc
        local.get $n
        i32.const 1
        i32.sub
        local.set $n
        br $top))
    local.get $acc)

  (func $foldedif (param $x i32) (result i32)
    (if (result i32) (local.get $x)
      (then (i32.const 1))
      (else (i32.const 0))))

  (func $flatif (param $x i32) (result i32)
    local.get $x
    if (result i32)
      i32.const 1
    else
      i32.const 0
    end)

  (func $mem (param $addr i32) (result i32)
    local.get $addr
    i32.load offset=4 align=4
    local.get $addr
    i32.const 1
    i32.store offset=0)

  (func $callit (param $f i32) (result i32)
    i32.const 1
    i32.const 2
    local.get $f
    call_indirect (type $binop))

  (func $convs (param $x f64) (result i32)
    local.get $x
    i32.trunc_f64_s)

  (func $ops (result i32)
    i32.const -3
    i32.clz
    i64.const 0x10
    i64.eqz
    drop
    f32.const 1.5
    f32.neg
    f64.const 1.0e3
    f64.abs
    drop
    drop
    i32.const 1
    i32.const 2
    i32.ne
    drop
    unreachable)
)
