library language_features;

import 'dart:math' show pi, sqrt;
export 'dart:core';

typedef IntPredicate = bool Function(int value);
typedef Pair<A, B> = List<Object?>;

enum Color { red, green, blue }

mixin Flies {
  double altitude = 0;
  void fly() => altitude += 1;
}

class Point {
  final double x;
  final double y;
  static int instances = 0;

  Point(this.x, this.y) : assert(x.isFinite) {
    instances++;
  }

  Point.named({required double x, required double y}) : this(x, y);

  Point.origin() : this(0, 0);

  factory Point.polar(double radius, double angle) {
    return Point(radius * sqrt(2), angle);
  }

  double get magnitude => sqrt(x * x + y * y);
  set magnitude(double m) {}

  Point operator +(Point other) => Point(x + other.x, y + other.y);

  @override
  String toString() => 'Point($x, $y)';
}

class Bird extends Point with Flies {
  Bird(double x, double y) : super(x, y);
}

extension NumberParsing on String {
  int? get asInt => int.tryParse(this);
}

abstract class Shape {
  double area();
}

class Circle implements Shape {
  final double radius;
  const Circle(this.radius);

  @override
  double area() => pi * radius * radius;
}

int describe(Color c) {
  switch (c) {
    case Color.red:
      return 0;
    case Color.green:
    case Color.blue:
      return 1;
    default:
      return -1;
  }
}

String classify(Object? value) {
  if (value is int && value > 0) {
    return 'positive int';
  } else if (value is String) {
    final String s = value as String;
    return 'string of length ${s.length}';
  }
  return 'other';
}

List<int> merge(List<int> a, List<int>? b) {
  return [...a, if (b != null) ...b, for (var i in a) if (i.isEven) i * 2];
}

void loops() {
  outer:
  for (var i = 0; i < 3; i++) {
    for (var j = 0; j < 3; j++) {
      if (j == 1) continue outer;
      if (i == 2) break outer;
    }
  }
}

void errors() {
  try {
    throw StateError('boom');
  } on ArgumentError catch (e) {
    print(e);
  } on StateError catch (e, st) {
    print('$e\n$st');
  } catch (e) {
    rethrow;
  } finally {
    print('done');
  }
}

void cascadesAndNullAware(Point? maybe) {
  final Point p = Point(1, 2)
    ..toString()
    ..toString();
  final double? m = maybe?.magnitude;
  final double n = m ?? 0.0;
  var count = 0;
  count ??= 1;
  print('$p $n ${maybe!.x}');
  #symbolLiteral;
}

void main() {
  loops();
  errors();
  cascadesAndNullAware(Point.origin());
  print(describe(Color.blue));
  print(classify(42));
  print(merge([1, 2, 3], [4, 5]));
  print('42'.asInt);
  const IntPredicate isPositive = _isPositive;
}

bool _isPositive(int v) => v > 0;
