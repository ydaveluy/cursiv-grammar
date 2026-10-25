#include <typeinfo>

int keyword_expressions() {
  auto value = int(1);
  auto cast = static_cast<int>(1.0);
  auto const& info = typeid(int);
  return value + cast + (info == typeid(int));
}
