n 100;
msg "hello";
tbl 3 1, 2, 3;

count(x) {
	extrn n;
	auto i, sum;
	i = 0;
	sum = 0;
	while (i < x) {
		sum =+ i;
		i++;
	}
	return (sum);
}

classify(v) {
	auto r;
	switch (v) {
	case 0:
		r = 'zero';
	case 1:
		r = 'one';
	}
	r = (v > 0) ? 1 : -1;
	return (r);
}

main() {
	auto p, x;
	x = 5;
	p = &x;
	--x;
	if (!x) goto done;
	count(x);
done:
	classify(x);
}
