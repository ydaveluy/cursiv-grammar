int alternative_tokens(int a, int b) {
  a and_eq b;
  a or_eq b;
  a xor_eq b;
  auto capture = [bitand a] { return a; };
  auto default_capture = [bitand] { return a; };
  auto init_capture = [bitand value = a] { return value; };
  return compl a bitand b bitor (a xor b) + (a not_eq b);
}

int value;
int bitand lvalue_reference = value;
int and rvalue_reference = 1;

struct Alternative {
  int operator bitand(int) const;
  int operator and_eq(int);
  void member() bitand;
  void member() and;
};
