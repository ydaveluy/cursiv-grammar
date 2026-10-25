import a.b.c
import a.b.c as abc

pragma[noinline]
predicate helper(int x) {
  x = 1
}

language[monotonicAggregates]
predicate mono(int x) {
  x = 1
}

signature module Sig {
  class Elem;
}

int countAll(int lo, int hi) {
  result = strictcount(int i | i = [lo .. hi] | i)
}

int sumAll(int lo, int hi) {
  result = strictsum(int i | i = [lo .. hi] | i)
}

string concatAll(int lo, int hi) {
  result = strictconcat(int i | i = [lo .. hi] | i.toString())
}

predicate uniqueTest(int lo, int hi) {
  unique(int i | i = [lo .. hi] | i) = 1
}

predicate anyTest() {
  any(int i | i = [1 .. 3] | i) = 1
}

predicate noneTest() {
  none() instanceof int
}

predicate underscoreTest() {
  helper(_)
}

predicate leTest(int a, int b) {
  a <= b
}

int pragmaTest(int a) {
  result = pragma[only_bind_out](a)
}

predicate closureStar() {
  helper*(1)
}

predicate namedTest(int unique) {
  unique = 1
}

predicate forexTest() {
  forex(int i | i = [1 .. 3] | i > 0)
}

predicate existsExprTest(int x) {
  exists(x)
}
