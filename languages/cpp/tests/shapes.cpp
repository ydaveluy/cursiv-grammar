// Exercises constructs no upstream example does: a namespace, a class
// with a virtual base and access specifiers, a template class and function,
// an enum class, using-declarations and a try/catch.
namespace shapes {

enum class Kind { Circle, Square };

class Shape {
 public:
  Shape(Kind kind) : kind_(kind) {}
  virtual ~Shape() {}
  virtual double area() const = 0;

 protected:
  Kind kind_;
};

class Circle : public Shape {
 public:
  Circle(double r) : Shape(Kind::Circle), radius_(r) {}
  double area() const override { return 3.14159 * radius_ * radius_; }

 private:
  double radius_;
};

template <typename T>
class Box {
 public:
  explicit Box(T value) : value_(value) {}
  T const& get() const { return value_; }

 private:
  T value_;
};

template <typename T>
T maxOf(T a, T b) {
  return a > b ? a : b;
}

using shapes::Kind;

double totalArea(const Shape& a, const Shape& b) {
  try {
    return a.area() + b.area();
  } catch (const std::exception& e) {
    throw;
  } catch (...) {
    return 0.0;
  }
}

}  // namespace shapes

int main() {
  shapes::Circle c(2.0);
  shapes::Box<int> box(42);
  return static_cast<int>(shapes::maxOf(1, 2) + box.get());
}
