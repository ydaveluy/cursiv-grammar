/* Constructs the upstream corpus does not exercise: /* nested */ comments,
   every keyword_instruction, every operator, do_'s orderings, and every
   template item shape. */

/* %include somefile */

a: b: c = 1
say a b c

address value 'FOO'
address TSO
address

call foo 1, 2
call foo(1, 2)
call on error
call on error name handler
call off failure

drop a (b) c.
upper a (b)

interpret 'say 1'

numeric digits 9
numeric digits
numeric fuzz 3
numeric form engineering
numeric form scientific
numeric form value 9
numeric form 9

options 'foo'

parse arg t1 . t2 'pat' (varpat) 3 =4 =(posvar) +5 +(relvar) -6 -(relvar2)
parse value x with a b
parse var x a b
parse upper pull a .

pull a b
push 'x'
queue 'x'

procedure expose a b

signal on novalue
signal on syntax name handler
signal off error
signal value 'label1'

trace error
trace 'C'
trace off
trace value x

x = 1 + -2 * 3 ** 2
y = 1 // 2 % 3
z = a & b | c && d
w = \a
v = a == b \== c >> d << e >>= f <<= g \>> h \<< i
u = a = b \= c <> d >< e > f < g >= h <= i \> j \< k

do i = 1 to 10 by 2 for 3
  say i
end

do while x < 10
  x = x + 1
end

do until x > 0
  x = x - 1
end

do forever
  leave
end

do 5
  iterate
end

select
  when a = 1 then say 'one'
  when a = 2 then say 'two'
  otherwise say 'other'
end

if a = 1 then say 'yes'; else say 'no'

exit 0

label1:
return
