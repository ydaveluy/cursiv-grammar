// The wkt language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <cstdint>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace wkt::ast {

enum class Dim : std::uint8_t { none, z, m, zm };
enum class AngleUnit : std::uint8_t { degrees, gradians, radians };
enum class TinElementKind : std::uint8_t {
  points,
  groupSpot,
  boundary,
  breakline,
  softbreak,
  controlContour,
  breakVoid,
  drapeVoid,
  voidElement,
  hole,
  stopline
};

struct Point : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> ordinates;
};

struct Vector : cursiv::Extends<cursiv::AstNode> {
  boolean hasZ{};          // the VECTOR keyword's own Z_ marker
  Point* value = nullptr;  // 2-3 ordinates; null when EMPTY
};

// A bare "(p, p, ...)" run with no keyword of its own: a polygon ring, a
// multi-line-string's member, a bare line segment inside a curve. Zero
// points means the run came from EMPTY, not from an empty parenthesized
// list.
struct PointList : cursiv::Extends<cursiv::AstNode> {
  Dim dim{};
  vector<Point*> points;
};

// A bare polygon: a list of rings, no POLYGON keyword.
struct BarePolygon : cursiv::Extends<cursiv::AstNode> {
  Dim dim{};
  vector<PointList*> rings;
};

// One slot of a MULTIPOINT list: a point, or individually EMPTY.
struct MultiPointItem : cursiv::Extends<cursiv::AstNode> {
  Dim dim{};
  Point* value = nullptr;
};

struct Location : cursiv::Extends<cursiv::AstNode> {
  boolean hasZ{};
  Point* value = nullptr;
};

struct ReferenceDirections : cursiv::Extends<cursiv::AstNode> {
  vector<Vector*> vectors;
};

struct AffinePlacement : cursiv::Extends<cursiv::AstNode> {
  boolean hasZ{};
  Location* location = nullptr;
  ReferenceDirections* directions = nullptr;
};

struct ReferenceLocation : cursiv::Extends<cursiv::AstNode> {
  AffinePlacement* placement = nullptr;
};

struct Angle : cursiv::Extends<cursiv::AstNode> {
  AngleUnit unit{};
  double value{};
};

// Every alternative of the standard's <well-known text representation>,
// plus LINEARRING and the JTS clothoid it coexists with.
struct Geometry : cursiv::Extends<cursiv::AstNode> {
  Dim dim{};
};

struct PointGeom : cursiv::Extends<Geometry> {
  Point* value = nullptr;
};

// LINESTRING, LINEARRING, CIRCULARSTRING, CIRCLE, GEODESICSTRING: one
// keyword, an optional dim, then a bare point list -- kept as five classes
// so a reader's `type` tag still names which one it is.
struct PointListGeometry : cursiv::Extends<Geometry> {
  vector<Point*> points;
};
struct LineStringGeom : cursiv::Extends<PointListGeometry> {};
struct LinearRingGeom : cursiv::Extends<PointListGeometry> {};
struct CircularStringGeom : cursiv::Extends<PointListGeometry> {};
struct CircleGeom : cursiv::Extends<PointListGeometry> {};
struct GeodesicStringGeom : cursiv::Extends<PointListGeometry> {};

struct PolygonGeom : cursiv::Extends<Geometry> {
  vector<PointList*> rings;
};

struct TriangleGeom : cursiv::Extends<Geometry> {
  vector<Point*> corners;    // the bare "(p, p, p)" form
  vector<PointList*> rings;  // the wrapped-ring OGC form
};

struct MultiPointGeom : cursiv::Extends<Geometry> {
  vector<MultiPointItem*> items;
};

struct MultiLineStringGeom : cursiv::Extends<Geometry> {
  vector<PointList*> lines;
};

struct MultiPolygonGeom : cursiv::Extends<Geometry> {
  vector<BarePolygon*> polygons;
};

struct GeometryCollectionGeom : cursiv::Extends<Geometry> {
  vector<Geometry*> geometries;
};

// Namespace scope: the AstNode vocabulary is out of reach here, so
// `std::variant` is spelled explicitly.
using CurveMember = std::variant<PointList*, Geometry*>;
using SurfaceMember = std::variant<BarePolygon*, Geometry*>;
using PatchGeom = std::variant<PolygonGeom*, TriangleGeom*>;
using ShellMember = std::variant<struct PolyhedralSurfaceGeom*,
                                  struct CompoundSurfaceGeom*>;

struct CompoundCurveGeom : cursiv::Extends<Geometry> {
  vector<CurveMember> members;
};
struct CurvePolygonGeom : cursiv::Extends<Geometry> {
  vector<CurveMember> members;
};
struct MultiCurveGeom : cursiv::Extends<Geometry> {
  vector<CurveMember> members;
};

struct MultiSurfaceGeom : cursiv::Extends<Geometry> {
  vector<SurfaceMember> members;
};
struct CompoundSurfaceGeom : cursiv::Extends<Geometry> {
  vector<SurfaceMember> members;
};

struct TinElement : cursiv::Extends<cursiv::AstNode> {
  TinElementKind kind{};
  optional<double> id;
  string_view tag;
  variant<MultiPointGeom*, PolygonGeom*, TriangleGeom*, LineStringGeom*>
      payload;
};

struct PolyhedralSurfaceGeom : cursiv::Extends<Geometry> {
  vector<PatchGeom> patches;      // the PATCHES form
  vector<BarePolygon*> polygons;  // the bare polygon-list (OGC) form
};

struct TinGeom : cursiv::Extends<Geometry> {
  vector<TriangleGeom*> patchTriangles;  // PATCHES form: a bare triangle list
  vector<TinElement*> elements;
  optional<double> maxSideLength;
  vector<BarePolygon*> polygons;  // the bare polygon-list (OGC) form
};

struct BrepSolidGeom : cursiv::Extends<Geometry> {
  vector<ShellMember> shells;
};

struct EllipticalCurveGeom : cursiv::Extends<Geometry> {
  ReferenceLocation* referenceLocation = nullptr;
  optional<double> uAxisLength;
  optional<double> vAxisLength;
  Angle* startAngle = nullptr;
  Angle* endAngle = nullptr;
  optional<double> startM;
  optional<double> endM;
};

struct WeightedPoint : cursiv::Extends<cursiv::AstNode> {
  boolean hasZ{};
  Point* value = nullptr;
};
struct NurbsPoint : cursiv::Extends<cursiv::AstNode> {
  WeightedPoint* point = nullptr;
  optional<double> weight;
};
struct ControlPoints : cursiv::Extends<cursiv::AstNode> {
  boolean hasZ{};
  vector<NurbsPoint*> points;
};
struct Knot : cursiv::Extends<cursiv::AstNode> {
  optional<double> value;
  optional<double> multiplicity;
};
struct Knots : cursiv::Extends<cursiv::AstNode> {
  vector<Knot*> knots;
};
struct NurbsCurveGeom : cursiv::Extends<Geometry> {
  optional<double> degree;
  ControlPoints* controlPoints = nullptr;
  Knots* knots = nullptr;
  optional<double> startM;
  optional<double> endM;
};

// ISO/IEC 13249-3 5.1.67's placement-based form, and the (startKappa,
// endKappa, length) JTS extension nested inside a COMPOUNDCURVE: told
// apart by which fields a document sets.
struct ClothoidGeom : cursiv::Extends<Geometry> {
  ReferenceLocation* referenceLocation = nullptr;
  optional<double> scaleFactor;
  optional<double> startDistance;
  optional<double> endDistance;
  optional<double> startM;
  optional<double> endM;
  optional<double> startKappa;
  optional<double> endKappa;
  optional<double> length;
};

struct SpiralCurveGeom : cursiv::Extends<Geometry> {
  ReferenceLocation* referenceLocation = nullptr;
  optional<double> length;
  optional<double> startCurvature;
  optional<double> endCurvature;
  string_view spiralType;
  optional<double> startM;
  optional<double> endM;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Geometry*> geometries;
};

}  // namespace wkt::ast
