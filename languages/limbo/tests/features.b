implement Features;

include "sys.m";

Math: module
{
	PI: con 3;
	Add: fn(a, b: int): int;
};

Shape: adt
{
	x, y: int;
	area: fn(nil: self Shape): int;
	move: fn(nil: self ref Shape);
	pick
	{
	Circle or Square =>
		r: cyclic int;
	Empty =>
	}
};

(a, b) := f();

total: int = 0;

worker(n: int)
{
	exit;
}

main(argv: list of string)
{
	x, y: int;
	pi: con 3;
	m: import Math;
	t: type int;

	a := 1;
	a = 2;

	nums := array[3] of {1, 2, 3};
	sparse := array[] of {1 => 10, * => 0};
	lst := list of {1, 2, 3};
	c := chan of int;
	n := big 5;
	r := real 5;

	i := 0;
outer:
	for (i = 0; i < 10; i++)
	{
		if (i == 5)
			break outer;
		else
			continue outer;
	}

loop:
	while (i < 10)
		i = i + 1;

	do
		i--;
	while (i > 0);

	case i
	{
	1 or 2 =>
		x = 1;
	3 to 5 =>
		x = 2;
	* =>
		x = 0;
	}

	alt
	{
	c <-= 1 =>
		x = 1;
	}

	pick v := f()
	{
	Circle =>
		x = 1;
	* =>
		x = 0;
	}

	c <-= 1;
	v := <-c;

	s := a :: b :: c;
	p := nums[0:2];
	q := nums[1:];

	spawn worker(1);

	return;
}
