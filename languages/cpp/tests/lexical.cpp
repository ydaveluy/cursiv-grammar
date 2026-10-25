double whole = 0x1p2;
double fractional = 0x.8p+1;
double trailing = 0x1.p-2;
float separated = 0xA'B.C'Dp1'0f;
int π = 1;
int intπ = 2;

constexpr long double operator""_μ(long double value) { return value; }
auto suffixed = 1.0_μ;

[[]] int empty_attribute;
[[,,,,]] int empty_attributes;
[[, maybe_unused,,]] int sparse_attributes;

// Continued comment \
this text is still inside the comment
int after_comment;
