// The wavefront language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <cstdint>
#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace wavefront::ast {

enum class Axis : std::uint8_t { u, v };

struct Statement : cursiv::Extends<cursiv::AstNode> {};

// Vertex data
// -----------

struct Vertex : cursiv::Extends<Statement> {
  double x = 0;
  double y = 0;
  double z = 0;
  optional<double> w;
  vector<double> color;
};

struct VertexParameter : cursiv::Extends<Statement> {
  double u = 0;
  optional<double> v;
  optional<double> w;
};

struct VertexNormal : cursiv::Extends<Statement> {
  double x = 0;
  double y = 0;
  double z = 0;
};

struct VertexTexture : cursiv::Extends<Statement> {
  double u = 0;
  optional<double> v;
  optional<double> w;
};

// Free-form curve/surface attributes
// ----------------------------------

enum class CurveSurfaceKind : std::uint8_t {
  bmatrix,
  bezier,
  bspline,
  cardinal,
  taylor,
};

struct CurveSurfaceType : cursiv::Extends<Statement> {
  bool rational = false;
  CurveSurfaceKind kind{};
};

struct Degree : cursiv::Extends<Statement> {
  std::int64_t u = 0;
  optional<std::int64_t> v;
};

struct BasisMatrix : cursiv::Extends<Statement> {
  Axis axis{};
  vector<double> values;
};

struct Step : cursiv::Extends<Statement> {
  std::int64_t u = 0;
  optional<std::int64_t> v;
};

// Elements
// --------

struct ElementRef : cursiv::Extends<cursiv::AstNode> {
  string_view vertex;
  optional<string_view> texture;
  optional<string_view> normal;
};

struct Points : cursiv::Extends<Statement> {
  vector<ElementRef*> v;
};

struct Lines : cursiv::Extends<Statement> {
  vector<ElementRef*> v;
};

struct Faces : cursiv::Extends<Statement> {
  vector<ElementRef*> v;
};

// Free-form curve/surface body
// -----------------------------

struct FreeFormHead : cursiv::Extends<cursiv::AstNode> {};

struct Curve : cursiv::Extends<FreeFormHead> {
  double u0 = 0;
  double u1 = 0;
  vector<std::int64_t> v;
};

struct Curve2D : cursiv::Extends<FreeFormHead> {
  vector<std::int64_t> vp;
};

struct Surface : cursiv::Extends<FreeFormHead> {
  double s0 = 0;
  double s1 = 0;
  double t0 = 0;
  double t1 = 0;
  vector<ElementRef*> v;
};

struct FreeFormBodyItem : cursiv::Extends<cursiv::AstNode> {};

struct Parameter : cursiv::Extends<FreeFormBodyItem> {
  Axis axis{};
  vector<double> p;
};

// One shape (u0, u1, curv2d)+ covers `trim`, `hole` and `scrv` alike.
struct TrimSegment : cursiv::Extends<cursiv::AstNode> {
  double u0 = 0;
  double u1 = 0;
  std::int64_t curv2d = 0;
};

struct OuterTrimmingLoop : cursiv::Extends<FreeFormBodyItem> {
  vector<TrimSegment*> segments;
};

struct InnerTrimmingLoop : cursiv::Extends<FreeFormBodyItem> {
  vector<TrimSegment*> segments;
};

struct SpecialCurve : cursiv::Extends<FreeFormBodyItem> {
  vector<TrimSegment*> segments;
};

struct SpecialPoint : cursiv::Extends<FreeFormBodyItem> {
  vector<std::int64_t> vp;
};

struct FreeFormSurface : cursiv::Extends<Statement> {
  FreeFormHead* head = nullptr;
  vector<FreeFormBodyItem*> body;
};

// Connectivity between free-form surfaces
// ---------------------------------------

struct Connectivity : cursiv::Extends<Statement> {
  std::int64_t surf1 = 0;
  double q0_1 = 0;
  double q1_1 = 0;
  std::int64_t curv2d1 = 0;
  std::int64_t surf2 = 0;
  double q0_2 = 0;
  double q1_2 = 0;
  std::int64_t curv2d2 = 0;
};

// Grouping
// --------

struct Group : cursiv::Extends<Statement> {
  vector<std::string_view> names;
};

struct SmoothingGroup : cursiv::Extends<Statement> {
  bool off = false;
  optional<std::int64_t> groupNumber;
};

struct MergingGroup : cursiv::Extends<Statement> {
  bool off = false;
  optional<std::int64_t> groupNumber;
  optional<double> res;
};

struct ObjectName : cursiv::Extends<Statement> {
  string_view name;
};

// Display/render attributes
// --------------------------

struct Bevel : cursiv::Extends<Statement> {
  bool on = false;
};

struct ColorInterpolation : cursiv::Extends<Statement> {
  bool on = false;
};

struct DissolveInterpolation : cursiv::Extends<Statement> {
  bool on = false;
};

struct LevelOfDetail : cursiv::Extends<Statement> {
  std::int64_t level = 0;
};

struct MapLibrary : cursiv::Extends<Statement> {
  vector<std::string_view> filenames;
};

struct UseMap : cursiv::Extends<Statement> {
  bool off = false;
  optional<std::string_view> name;
};

struct MaterialLibrary : cursiv::Extends<Statement> {
  vector<string_view> filenames;
};

struct UseMaterial : cursiv::Extends<Statement> {
  string_view name;
};

struct ShadowObject : cursiv::Extends<Statement> {
  string_view filename;
};

struct TraceObject : cursiv::Extends<Statement> {
  string_view filename;
};

// Approximation techniques
// -------------------------

struct CurveApproxTechnique : cursiv::Extends<Statement> {};

struct CtechCparm : cursiv::Extends<CurveApproxTechnique> {
  double res = 0;
};

struct CtechCspace : cursiv::Extends<CurveApproxTechnique> {
  double maxlength = 0;
};

struct CtechCurv : cursiv::Extends<CurveApproxTechnique> {
  double maxdist = 0;
  double maxangle = 0;
};

struct SurfaceApproxTechnique : cursiv::Extends<Statement> {};

struct StechCparma : cursiv::Extends<SurfaceApproxTechnique> {
  double ures = 0;
  double vres = 0;
};

struct StechCparmb : cursiv::Extends<SurfaceApproxTechnique> {
  double uvres = 0;
};

struct StechCspace : cursiv::Extends<SurfaceApproxTechnique> {
  double maxlength = 0;
};

struct StechCurv : cursiv::Extends<SurfaceApproxTechnique> {
  double maxdist = 0;
  double maxangle = 0;
};

// General statement
// ------------------

struct Call : cursiv::Extends<Statement> {
  string_view filename;
  vector<std::string_view> args;
};

struct Csh : cursiv::Extends<Statement> {
  string_view command;
  bool dash = false;
  vector<std::string_view> args;
};

struct OtherStatement : cursiv::Extends<Statement> {
  string_view keyword;
  string_view arguments;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Statement*> statements;
};

}  // namespace wavefront::ast
