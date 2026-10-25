@rmproperty oldprop
@kids somename
@parent someparent
@disp Foo.bar
@notedit Foo.bar
@edit Foo.bar
@addalias alias1, alias2 to Foo.bar

@program tester:run
if (1 == 1 && 2 < 3)
  xx = 1;
elseif (1 != 2 || 3 >= 3)
  xx = 2;
else
  xx = 3;
endif;
try
  yy = foo(1, 2.5, #12, $, -1, +2, !xx);
except Foo.bar
  yy = 0;
endtry
do
  xx = xx + 1;
while (xx <= 5)
for item in {1, 2, 3}
  yy = yy + item;
endfor
while (xx < 10)
  xx = xx + 1;
endwhile
.
