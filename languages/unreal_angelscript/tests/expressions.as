// Own test document: the full expression ladder, casts, constructs,
// generics, asserts and the supported literal shapes
// of the language.

void TestExpressions()
{
	int32 a = 1 + 2 * 3 - 4 / 2 % 2;
	int32 b = (1 << 2) >> 1;
	bool c = (a < b) && (a > b) || (a <= b) && (a >= b);
	bool d = (a == b) != (a != b);
	int32 e = a & b | a ^ b;
	int32 f = a ^^ b;
	int32 g = -a + +a - ~a;
	bool h = !c;
	int32 i2 = a;
	int32 j = a;
	int32 k = a++ + --a;
	int32 l = a > b ? a : b;

	a += 1;
	a -= 1;
	a *= 2;
	a /= 2;
	a %= 2;
	a <<= 1;
	a >>= 1;
	a &= 1;
	a ^= 1;
	a |= 1;

	int32 m = a + b + a;

	int32 n = int(3.5f);
	float o = float(n);

	TArray<int32> arr;
	TMap<FString, int32> map;

	auto p = Cast<ABaseThing>(a);


	ensure(a > 0);
	ensureAlways(a > 0);
	check(a != 0);

	int32 declTyped = 10;

	string s1 = "hello";
	string s2 = """multi
line""";
	string s3 = n"narrow";
	string s4 = f"formatted";
	char cv = 'x';

	int32 hexv = 0x1F;
	int32 octv = 0o17;
	int32 binv = 0b101;
	int32 sepv = 1000000;
	float fv = 1.5e10f;

	int32 plainValue = 123;

	bool isNull = (arr == nullptr);
}
