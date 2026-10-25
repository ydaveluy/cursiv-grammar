// Expressions: every literal shape, scoped and rooted names, a call, a
// construction call (plain and generic), a cast, a lambda, both typed and
// untyped/nested/sparse initialiser lists, every prefix and postfix operator
// (member, method call, index with named arguments, a bare call postfix,
// increment/decrement), the full operator ladder including the two- and
// three-character shift spellings next to closing generics, the ternary and
// every compound assignment.

class Vec
{
    float x, y;
    Vec(float ax, float ay) { x = ax; y = ay; }
    float dot(Vec other) { return x * other.x + y * other.y; }
}

funcdef void Handler(int code);

void showcase()
{
    // literals
    int dec = 10;
    float pi = 3.14;
    // A string literal has no escape mechanism: a quote inside one is
    // written with the other delimiter instead.
    string greeting = "she said 'hello'";
    string tag = 'a "quoted" word';
    uint mask = 0xff01;
    uint bits = 0b1010;
    uint oct = 0o17;
    bool flag = true;
    bool other = false;
    Vec@ nothing = null;

    // scoped and rooted names
    ::globalCounter = ::globalCounter + 1;
    Game::EntityId id = 0;

    // a call and a plain construction call (both spelled `name(args)`)
    notify(1);
    Vec v = Vec(1.0, 2.0);

    // a generic construction call
    array<int> nums(3);

    // cast
    int truncated = cast<int>(pi);

    // a lambda
    Handler@ h = function(int code) { globalCounter = code; };

    // initialiser lists: untyped, typed, nested, and a sparse slot
    int[] xs = {1, 2, 3};
    array<int> ys = array<int> = {1, , 3};
    int[][] grid = {{1, 2}, {3, 4}};

    // postfix chain: member, method call, index with a named argument,
    // increment
    Vec w = Vec(0.0, 0.0);
    float wx = w.x;
    float d = w.dot(v);
    nums[0] = nums[index: 1];
    dec++;
    --dec;

    // a bare call postfix: calling the value a call produced
    Handler@ picked = h;
    picked(2);

    // a zero-argument call
    globalTick();

    // prefix operators
    int neg = -dec;
    int pos = +dec;
    bool notFlag = !flag;
    bool notFlag2 = not flag;
    int comp = ~mask;
    Vec@ handle = @v;

    // the operator ladder, including shift next to closing generics
    array<array<int>> nested(1);
    bool closed = nested.length() > 0;
    uint shiftedLeft = bits << 2;
    uint shiftedRight = mask >> 1;
    uint shiftedUnsigned = mask >>> 1;
    bool cmp = dec < 20 && pi <= 4.0 || dec >= 5;
    bool eq = dec == 10 && dec != 11;
    bool logic = flag and other or (flag xor other);
    int bitwise = (dec & 1) | (dec ^ 2);
    bool identity = v is v;
    bool notIdentity = v !is nothing;
    int quirky = dec % 3;

    // precedence probes: * binds tighter than +, ** chains left-to-right,
    // & binds tighter than == (AngelScript ranks the bitwise operators
    // above the comparisons, unlike C), and shift binds tighter than a
    // relational op though both open on `<`/`>`
    int precMulAdd = 1 + 2 * 3;
    int precPow = 2 ** 3 ** 2;
    bool precBitEq = 1 & 3 == 3;
    bool precShiftRel = 1 << 2 < 8;

    // a generic segment inside a scope prefix, e.g. `Holder<int>::Make(...)`
    int viaGeneric = Holder<int>::Make(1);

    // ternary -- a variable declarator's own initialiser reads a plain
    // expression, not the full ternary/assignment chain, so the ternary
    // form here needs an assignment statement instead of an initialiser.
    int picked2;
    picked2 = flag ? dec : -dec;

    // every compound assignment
    int acc = 0;
    acc += 1;
    acc -= 1;
    acc *= 2;
    acc /= 2;
    acc %= 3;
    acc **= 2;
    acc <<= 1;
    acc >>= 1;
    acc >>>= 1;
    mask |= 1;
    mask &= 0xff;
    mask ^= 2;
}
