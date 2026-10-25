// Coverage for constructs the two-file upstream corpus never exercises:
// partial definitions, includes, typedef, callbacks, iterable/maplike/
// setlike, every extended-attribute form, stringifiers, static members,
// variadic and optional arguments with defaults, union types (plain and
// nested), nullable types, sequence<>/record<>/Promise<>/FrozenArray<>/
// ObservableArray<>, and every primitive-type spelling.

[NoArgsAttr, ArgListAttr(long x), IdentAttr=Bar, NamedArgListAttr=Baz(long y),
 IdentListAttr=(A, B, C), StringAttr="hello", StringListAttr=("a", "b", "c")]
interface ExtAttrShowcase {
  undefined method();
};

typedef unsigned long long BigCounter;
typedef unrestricted double Wobbly;
typedef (Node or DOMString) NodeOrString;
typedef ((long or DOMString) or boolean)? NestedUnion;
typedef sequence<DOMString> StringList;
typedef record<DOMString, long> Counters;
typedef Promise<undefined> VoidPromise;

callback AsyncCallback = undefined (DOMString reason);

interface Shape {
  constructor();
  constructor(double size);

  const long DEFAULT_SIZE = 10;
  const boolean DEBUG = false;
  const double EPSILON = 0.001;

  attribute double size;
  attribute [Clamp] long clamped;
  readonly attribute DOMString kind;
  inherit attribute boolean visible;
  stringifier attribute DOMString label;
  static attribute long instanceCount;

  undefined resize(double factor);
  stringifier DOMString toDisplayString();
  static Shape create(optional DOMString kind = "generic");

  getter double (unsigned long index);
  setter undefined (unsigned long index, double value);
  deleter undefined (unsigned long index);

  iterable<double>;
  readonly maplike<DOMString, double>;
  readonly setlike<DOMString>;

  undefined draw(double x,
                 double y,
                 optional double scale = 1.0,
                 optional [Clamp] long depth = 0,
                 DOMString... labels);
  undefined addAll(sequence<double> values);
  undefined merge(record<DOMString, double> values);
  undefined accept((DOMString or double) value);
  undefined acceptNullable(DOMString? value);
  undefined acceptUnionNullable((DOMString or double)? value);
  Promise<boolean> ready();

  [CEReactions, Exposed=Window] undefined reflow();
};

partial interface Shape {
  attribute DOMString extra;
};

interface mixin ShapeMixin {
  const long MIXIN_CONST = 1;
  undefined mixinMethod();
  stringifier;
  readonly attribute DOMString mixinLabel;
};

Shape includes ShapeMixin;

namespace ShapeUtils {
  double area(Shape s);
  readonly attribute DOMString version;
  const long VERSION_MAJOR = 1;
};

partial namespace ShapeUtils {
  double perimeter(Shape s);
};

dictionary ShapeInit {
  required double size;
  DOMString kind = "generic";
  boolean visible = true;
  sequence<double> extra = [];
  ShapeInit nested = {};
  DOMString? note = null;
};

dictionary ShapeInitEx : ShapeInit {
  double weight;
};

partial dictionary ShapeInitEx {
  double density;
};

enum ShapeKind { "circle", "square", "triangle", };

callback interface EventListener2 {
  undefined handleEvent(DOMString eventType);
  const boolean CAPTURING_PHASE = true;
};

interface AsyncSource {
  async iterable<long>;
};

interface AsyncSourceKV {
  async iterable<DOMString, long>(optional long batchSize = 10);
};

interface PrimitiveHolder {
  attribute short a1;
  attribute long a2;
  attribute long long a3;
  attribute unsigned short a4;
  attribute unsigned long a5;
  attribute unsigned long long a6;
  attribute float a7;
  attribute unrestricted float a8;
  attribute double a9;
  attribute unrestricted double a10;
  attribute boolean a11;
  attribute byte a12;
  attribute octet a13;
  attribute bigint a14;
  attribute undefined a15;
  attribute DOMString a16;
  attribute ByteString a17;
  attribute USVString a18;
  attribute object a19;
  attribute symbol a20;
  attribute any a21;
  attribute ArrayBuffer a22;
  attribute DataView a23;
  attribute Int8Array a24;
  attribute Int16Array a25;
  attribute Int32Array a26;
  attribute Uint8Array a27;
  attribute Uint16Array a28;
  attribute Uint32Array a29;
  attribute Uint8ClampedArray a30;
  attribute Float32Array a31;
  attribute Float64Array a32;
  attribute FrozenArray<long> a33;
  attribute ObservableArray<long> a34;
  attribute (long or DOMString) a35;
  attribute (long or DOMString or boolean) a36;
  const float NEG_INF = -Infinity;
  const float POS_INF = Infinity;
  const float NOT_A_NUMBER = NaN;
  const long NEG_ONE = -1;
  const octet HEX = 0xFF;
};
