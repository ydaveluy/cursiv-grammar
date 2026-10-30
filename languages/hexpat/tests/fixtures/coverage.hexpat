using Pair<T> = Two<T>;

struct Header {
    u32 count;
    u32 declaredIn in;
    u32 declaredOut out;
    u32 atSection @ 0x10 in ram;
};

bitfield Flags {
    a : 1;
    padding : 3;
    b : 4;
};

fn classify(s32 x) {
    if (x < 0)
        return "negative";
    return x == 0 ? "zero" : "positive";
};

fn sum(u32 n) {
    u32 total = 0;
    for (u32 i = 0, i < n, i = i + 1) {
        total = total + i;
    }
    return total;
};

fn describe(auto value) {
    try {
        return std::format("{}", value);
    } catch {
        return "?";
    }
};

struct Body {
    try {
        u32 a;
    } catch {
        padding[4];
    }
};

Header header @ 0x00;
u32 sizeofExpr = sizeof(header);
u32 sizeofType = sizeof(u32);
str typeName = typenameof(header);
