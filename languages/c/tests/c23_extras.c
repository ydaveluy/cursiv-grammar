enum Weekday : int { MON, TUE, WED };

struct [[nodiscard]] Point {
  int x, y;
};

[[nodiscard]] int area(struct Point p) {
  static_assert(1 == 1, "sanity");
  _Static_assert(sizeof(int) > 0);
  bool ok = true;
  _Bool old_ok = false;
  void *nothing = nullptr;
  int n = alignof(struct Point);
  int m = _Alignof(int);
  _Alignas(16) char buf[16];
  alignas(int) char aligned_buf[4];
  typeof(p.x) width = p.x;
  return _Generic(width, int: width, default: 0) * n * m * ok - old_ok +
         (int)(nothing == 0) + buf[0] + aligned_buf[0];
}
