struct Header {
  int kind;
};

/* __attribute__, __extension__, _Atomic(T), a GNU case range, a compound
   literal, and a computed goto. */
__extension__ typedef long long trace_id_t;

int classify(int kind) __attribute__((noreturn));

_Atomic(int) counter;

int use_compound_literal(void) {
  struct Header *h = &(struct Header){.kind = 1};
  return h->kind;
}

int dispatch(int kind) {
  static void *table[] = {&&slow, &&fast};
  goto *table[kind == 0];
slow:
  return 0;
fast:
  return 1;
}

int classify_range(int kind) {
  switch (kind) {
    case 1 ... 9:
      return 1;
    default:
      return 0;
  }
}

/* `_Countof`/`_Maxof`/`_Minof` and `__real__`/`__imag__`, GCC extensions. */
int gcc_only(int a[10], double c) {
  return _Countof(a) + _Maxof(int) + _Minof(int) + (int)__real__ c;
}
