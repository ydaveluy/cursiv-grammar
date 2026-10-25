# A method needs neither an explicit `@N` ordinal (`ping`) nor a `->` return
# (`poke`); a generic argument may itself carry a further application
# (`Multi(UInt32)(Text)`).

@0xaaaaaaaaaaaaaaaa;

struct Wrapper(T) {
  value @0 :T;
}

struct Multi(A, B) {
  a @0 :A;
  b @1 :B;
}

struct Chained {
  x @0 :Wrapper(Multi(UInt32)(Text));
}

interface Misc {
  ping Void;
  poke (x :Int32);
}
