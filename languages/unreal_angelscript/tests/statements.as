// Own test document: every statement form -- if/else, switch/case/default
// fallthrough, while, do-while, both for-loop shapes, labels/goto, jump
// statements, blocks and a local declaration.

void TestStatements()
{
	int32 x = 0;

	if (x > 0)
		x = 1;
	else
		x = -1;

	switch (x)
	{
	case 1:
		x = 2;
		break;
	default:
		x = 3;
		break;
	}

	while (x < 10)
	{
		x++;
	}

	do
	{
		x--;
	}
	while (x > 0);

	for (int32 i = 0; i < 10; i++)
	{
		if (i == 5)
			continue;
		if (i == 8)
			break;
	}

	int32 arr;
	for (int32 v : arr)
	{
		x += v;
	}

Loop:
	x = x + 1;
	goto Loop;

	;

	return;
}
