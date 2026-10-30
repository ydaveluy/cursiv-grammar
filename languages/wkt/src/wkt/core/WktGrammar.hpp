// The wkt language: its grammar.
#pragma once

#include <limits>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "wkt/core/ast.hpp"
#include "wkt/core/Numbers.hpp"

namespace wkt {

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};

// SPIRAL_TYPE_NAME's leading trivia is already gone by the time the token
// starts; only a trailing run before the closing `,`/`)` needs trimming.
inline constexpr auto trim_trailing_ws = [](std::string_view t) {
  while (!t.empty() && (t.back() == ' ' || t.back() == '\t' ||
                        t.back() == '\r' || t.back() == '\n'))
    t.remove_suffix(1);
  return t;
};

struct Wkt : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  struct DECIMAL
      : terminal<as<double>(
            option(cr<"+-">) +
            ((some(d) + option(lit<"."> + many(d))) | (lit<"."> + some(d))) +
            option(cr<"eE"> + option(cr<"+-">) + some(d)))> {};
  inline static constexpr auto special_value = [](std::string_view text) {
    return *ordinate_value(text);
  };
  struct SPECIAL_NUMBER
      : terminal<as<special_value>(
            option(cr<"+-">) + (ikw<"NAN"> | ikw<"INF"> | ikw<"INFINITY">))> {};
  static constexpr auto Ordinate = ref<DECIMAL> | ref<SPECIAL_NUMBER>;
  struct NUMBER : terminal<some(cr<R"(^ \t\r\n(),)">)> {};

  template<cursiv::str Text>
  static constexpr auto GeometryKeyword = cursiv::Lit<Text, true>{} +
      &(option(ikw<"ZM"> | ikw<"Z"> | ikw<"M">) + !w);

  struct EMPTY_ : terminal<ikw<"EMPTY">> {};
  struct Z_ : terminal<as<ast::Dim::z>(ikw<"Z">)> {};
  static constexpr auto DimSpec = as<ast::Dim::zm>(ikw<"ZM">) | ref<Z_> | as<ast::Dim::m>(ikw<"M">);
  static constexpr auto DimClause = option(assign<&ast::Geometry::dim>(DimSpec));

  struct QUOTED_LETTERS
      : terminal<as<unquote>(lit<"\""> + some(cr<"a-zA-Z0-9()_.' -">) +
                             lit<"\"">)> {};

  // Every geometry-producing rule is forward declared once, up front, so the
  // choices below (a curve member nesting COMPOUNDCURVE, a surface member
  // nesting COMPOUNDSURFACE, GEOMETRYCOLLECTION nesting `geometry` itself)
  // can name each other regardless of definition order.
  struct PointGeometry;
  struct LineStringGeometry;
  struct LinearRingGeometry;
  struct CircularStringGeometry;
  struct CircleGeometry;
  struct GeodesicStringGeometry;
  struct EllipticalCurveGeometry;
  struct NurbsCurveGeometry;
  struct ClothoidGeometry;
  struct SpiralCurveGeometry;
  struct CompoundCurveGeometry;
  struct PolygonGeometry;
  struct TriangleGeometry;
  struct CurvePolygonGeometry;
  struct PolyhedralSurfaceGeometry;
  struct TinGeometry;
  struct CompoundSurfaceGeometry;
  struct BrepSolidGeometry;
  struct MultiPointGeometry;
  struct MultiLineStringGeometry;
  struct MultiPolygonGeometry;
  struct MultiCurveGeometry;
  struct MultiSurfaceGeometry;
  struct GeometryCollection;

  struct Point
      : rule<create<ast::Point> +
             many(append<&ast::Point::ordinates>(ref<NUMBER>))> {};

  // A bare "(p, p, ...)" run, used wherever the standard has no keyword of
  // its own: a polygon ring, a multi-line-string's member, a bare curve
  // segment. Zero points means the run came from EMPTY.
  static constexpr auto PointListBodyText =
      (lit<"("> +
       some(append<&ast::PointList::points>(ref<Point>), lit<",">) +
       lit<")">) |
      ref<EMPTY_>;
  struct PointList : rule<create<ast::PointList> +
                              option(assign<&ast::PointList::dim>(DimSpec)) +
                              PointListBodyText> {};
  struct PlainPointList : rule<create<ast::PointList> + PointListBodyText> {
  };

  static constexpr auto BarePolygonBody =
      (lit<"("> +
       some(append<&ast::BarePolygon::rings>(ref<PointList>), lit<",">) +
       lit<")">) |
      ref<EMPTY_>;
  struct BarePolygon
      : rule<create<ast::BarePolygon> +
             option(assign<&ast::BarePolygon::dim>(DimSpec)) +
             BarePolygonBody> {};
  struct PlainPolygon : rule<create<ast::BarePolygon> + BarePolygonBody> {};

  struct MultiPointItem
      : rule<create<ast::MultiPointItem> +
             option(assign<&ast::MultiPointItem::dim>(DimSpec)) +
             (assign<&ast::MultiPointItem::value>(ref<Point>) |
              (lit<"("> + assign<&ast::MultiPointItem::value>(ref<Point>) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct LOCATION_ : terminal<ikw<"LOCATION">> {};
  struct AFFINEPLACEMENT_ : terminal<ikw<"AFFINEPLACEMENT">> {};
  struct REFERENCEDIRECTIONS_ : terminal<ikw<"REFERENCEDIRECTIONS">> {};
  struct VECTOR_ : terminal<ikw<"VECTOR">> {};
  struct REFERENCELOCATION_ : terminal<ikw<"REFERENCELOCATION">> {};

  struct Location
      : rule<create<ast::Location> + ref<LOCATION_> +
             option(flag<&ast::Location::hasZ>(ref<Z_>)) +
             ((lit<"("> + assign<&ast::Location::value>(ref<Point>) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct VectorEntry
      : rule<create<ast::Vector> + ref<VECTOR_> +
             option(flag<&ast::Vector::hasZ>(ref<Z_>)) +
             ((lit<"("> + assign<&ast::Vector::value>(ref<Point>) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct ReferenceDirections
      : rule<create<ast::ReferenceDirections> + ref<REFERENCEDIRECTIONS_> +
             ((lit<"("> +
               some(append<&ast::ReferenceDirections::vectors>(
                        ref<VectorEntry>),
                    lit<",">) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct AffinePlacement
      : rule<create<ast::AffinePlacement> + ref<AFFINEPLACEMENT_> +
             option(flag<&ast::AffinePlacement::hasZ>(ref<Z_>)) +
             ((lit<"("> +
               assign<&ast::AffinePlacement::location>(ref<Location>) +
               lit<","> +
               assign<&ast::AffinePlacement::directions>(
                   ref<ReferenceDirections>) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct ReferenceLocation
      : rule<create<ast::ReferenceLocation> + ref<REFERENCELOCATION_> +
             assign<&ast::ReferenceLocation::placement>(
                 ref<AffinePlacement>)> {};


  struct Angle
      : rule<create<ast::Angle> +
             assign<&ast::Angle::unit>(as<ast::AngleUnit::degrees>(ikw<"DEGREES">) | as<ast::AngleUnit::gradians>(ikw<"GRADIANS">) |
                                       as<ast::AngleUnit::radians>(ikw<"RADIANS">)) +
             lit<"("> + assign<&ast::Angle::value>(ref<DECIMAL>) + lit<")">> {
  };

  struct STARTM_ : terminal<ikw<"STARTM">> {};
  struct ENDM_ : terminal<ikw<"ENDM">> {};

  struct POINT : terminal<GeometryKeyword<"POINT">> {};
  struct LINESTRING : terminal<GeometryKeyword<"LINESTRING">> {};
  struct LINEARRING : terminal<GeometryKeyword<"LINEARRING">> {};
  struct CIRCULARSTRING : terminal<GeometryKeyword<"CIRCULARSTRING">> {};
  struct CIRCLE : terminal<GeometryKeyword<"CIRCLE">> {};
  struct GEODESICSTRING : terminal<GeometryKeyword<"GEODESICSTRING">> {};
  struct POLYGON : terminal<GeometryKeyword<"POLYGON">> {};
  struct TRIANGLE : terminal<GeometryKeyword<"TRIANGLE">> {};
  struct MULTIPOINT : terminal<GeometryKeyword<"MULTIPOINT">> {};
  struct MULTILINESTRING : terminal<GeometryKeyword<"MULTILINESTRING">> {};
  struct MULTIPOLYGON : terminal<GeometryKeyword<"MULTIPOLYGON">> {};
  struct GEOMETRYCOLLECTION : terminal<GeometryKeyword<"GEOMETRYCOLLECTION">> {};
  struct ELLIPTICALCURVE : terminal<GeometryKeyword<"ELLIPTICALCURVE">> {};
  struct NURBSCURVE : terminal<GeometryKeyword<"NURBSCURVE">> {};
  struct CLOTHOID : terminal<GeometryKeyword<"CLOTHOID">> {};
  struct SPIRALCURVE : terminal<GeometryKeyword<"SPIRALCURVE">> {};
  struct COMPOUNDCURVE : terminal<GeometryKeyword<"COMPOUNDCURVE">> {};
  struct CURVEPOLYGON : terminal<GeometryKeyword<"CURVEPOLYGON">> {};
  struct MULTICURVE : terminal<GeometryKeyword<"MULTICURVE">> {};
  struct MULTISURFACE : terminal<GeometryKeyword<"MULTISURFACE">> {};
  struct POLYHEDRALSURFACE : terminal<GeometryKeyword<"POLYHEDRALSURFACE">> {};
  struct COMPOUNDSURFACE : terminal<GeometryKeyword<"COMPOUNDSURFACE">> {};
  struct BREPSOLID : terminal<GeometryKeyword<"BREPSOLID">> {};
  struct TIN : terminal<GeometryKeyword<"TIN">> {};

  struct PointGeometry
      : rule<create<ast::PointGeom> + ref<POINT> + DimClause +
             ((lit<"("> + assign<&ast::PointGeom::value>(ref<Point>) +
               lit<")">) |
              ref<EMPTY_>)> {};

  static constexpr auto PointListBody =
      (lit<"("> +
       some(append<&ast::PointListGeometry::points>(ref<Point>), lit<",">) +
       lit<")">) |
      ref<EMPTY_>;

  struct LineStringGeometry
      : rule<create<ast::LineStringGeom> + ref<LINESTRING> + DimClause +
             PointListBody> {};
  struct LinearRingGeometry
      : rule<create<ast::LinearRingGeom> + ref<LINEARRING> + DimClause +
             PointListBody> {};
  struct CircularStringGeometry
      : rule<create<ast::CircularStringGeom> + ref<CIRCULARSTRING> +
             DimClause + PointListBody> {};
  struct CircleGeometry
      : rule<create<ast::CircleGeom> + ref<CIRCLE> + DimClause +
             PointListBody> {};
  struct GeodesicStringGeometry
      : rule<create<ast::GeodesicStringGeom> + ref<GEODESICSTRING> +
             DimClause + PointListBody> {};

  struct PolygonGeometry
      : rule<create<ast::PolygonGeom> + ref<POLYGON> + DimClause +
             ((lit<"("> +
               some(append<&ast::PolygonGeom::rings>(ref<PointList>),
                    lit<",">) +
               lit<")">) |
              ref<EMPTY_>)> {};

  static constexpr auto TriangleBody =
      (lit<"("> + append<&ast::TriangleGeom::corners>(ref<Point>) +
       lit<","> + append<&ast::TriangleGeom::corners>(ref<Point>) +
       lit<","> + append<&ast::TriangleGeom::corners>(ref<Point>) +
       lit<")">) |
      (lit<"("> +
       some(append<&ast::TriangleGeom::rings>(ref<PointList>), lit<",">) +
       lit<")">) |
      ref<EMPTY_>;

  struct TriangleGeometry
      : rule<create<ast::TriangleGeom> + ref<TRIANGLE> + DimClause +
             TriangleBody> {};
  // A TIN's PATCHES list: the same body as TriangleGeometry, no
  // TRIANGLE keyword.
  struct BareTriangle : rule<create<ast::TriangleGeom> + TriangleBody> {};

  struct MultiPointGeometry
      : rule<create<ast::MultiPointGeom> + ref<MULTIPOINT> + DimClause +
             ((lit<"("> +
               some(append<&ast::MultiPointGeom::items>(
                        ref<MultiPointItem>),
                    lit<",">) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct MultiLineStringGeometry
      : rule<create<ast::MultiLineStringGeom> + ref<MULTILINESTRING> +
             DimClause +
             ((lit<"("> +
               some(append<&ast::MultiLineStringGeom::lines>(
                        ref<PointList>),
                    lit<",">) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct MultiPolygonGeometry
      : rule<create<ast::MultiPolygonGeom> + ref<MULTIPOLYGON> + DimClause +
             ((lit<"("> +
               some(append<&ast::MultiPolygonGeom::polygons>(
                        ref<BarePolygon>),
                    lit<",">) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct UAXISLENGTH_ : terminal<ikw<"UAXISLENGTH">> {};
  struct VAXISLENGTH_ : terminal<ikw<"VAXISLENGTH">> {};
  struct STARTANGLE_ : terminal<ikw<"STARTANGLE">> {};
  struct ENDANGLE_ : terminal<ikw<"ENDANGLE">> {};

  struct EllipticalCurveGeometry
      : rule<create<ast::EllipticalCurveGeom> + ref<ELLIPTICALCURVE> +
             DimClause +
             ((lit<"("> +
               assign<&ast::EllipticalCurveGeom::referenceLocation>(
                   ref<ReferenceLocation>) +
               lit<","> + ref<UAXISLENGTH_> +
               (assign<&ast::EllipticalCurveGeom::uAxisLength>(ref<DECIMAL>) |
                ref<EMPTY_>) +
               lit<","> + ref<VAXISLENGTH_> +
               (assign<&ast::EllipticalCurveGeom::vAxisLength>(ref<DECIMAL>) |
                ref<EMPTY_>) +
               lit<","> + ref<STARTANGLE_> +
               assign<&ast::EllipticalCurveGeom::startAngle>(ref<Angle>) +
               lit<","> + ref<ENDANGLE_> +
               assign<&ast::EllipticalCurveGeom::endAngle>(ref<Angle>) +
               option(lit<","> + ref<STARTM_> +
                      assign<&ast::EllipticalCurveGeom::startM>(ref<DECIMAL>) +
                      lit<","> + ref<ENDM_> +
                      assign<&ast::EllipticalCurveGeom::endM>(ref<DECIMAL>)) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct DEGREE_ : terminal<ikw<"DEGREE">> {};
  struct CONTROLPOINTS_ : terminal<ikw<"CONTROLPOINTS">> {};
  struct NURBSPOINT_ : terminal<ikw<"NURBSPOINT">> {};
  struct WEIGHTEDPOINT_ : terminal<ikw<"WEIGHTEDPOINT">> {};
  struct WEIGHT_ : terminal<ikw<"WEIGHT">> {};
  struct KNOTS_ : terminal<ikw<"KNOTS">> {};
  struct KNOT_ : terminal<ikw<"KNOT">> {};
  struct VALUE_ : terminal<ikw<"VALUE">> {};
  struct MULTIPLICITY_ : terminal<ikw<"MULTIPLICITY">> {};

  struct WeightedPoint
      : rule<create<ast::WeightedPoint> + ref<WEIGHTEDPOINT_> +
             option(flag<&ast::WeightedPoint::hasZ>(ref<Z_>)) +
             ((lit<"("> + assign<&ast::WeightedPoint::value>(ref<Point>) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct NurbsPoint
      : rule<create<ast::NurbsPoint> + ref<NURBSPOINT_> +
             ((lit<"("> +
               assign<&ast::NurbsPoint::point>(ref<WeightedPoint>) +
               lit<","> + ref<WEIGHT_> +
               assign<&ast::NurbsPoint::weight>(ref<DECIMAL>) + lit<")">) |
              ref<EMPTY_>)> {};

  struct ControlPoints
      : rule<create<ast::ControlPoints> + ref<CONTROLPOINTS_> +
             option(flag<&ast::ControlPoints::hasZ>(ref<Z_>)) +
             ((lit<"("> +
               some(append<&ast::ControlPoints::points>(ref<NurbsPoint>),
                    lit<",">) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct Knot
      : rule<create<ast::Knot> + ref<KNOT_> +
             ((lit<"("> + ref<VALUE_> +
               assign<&ast::Knot::value>(ref<DECIMAL>) + lit<","> +
               ref<MULTIPLICITY_> +
               assign<&ast::Knot::multiplicity>(ref<DECIMAL>) + lit<")">) |
              ref<EMPTY_>)> {};

  struct Knots
      : rule<create<ast::Knots> + ref<KNOTS_> +
             ((lit<"("> +
               some(append<&ast::Knots::knots>(ref<Knot>), lit<",">) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct NurbsCurveGeometry
      : rule<create<ast::NurbsCurveGeom> + ref<NURBSCURVE> + DimClause +
             ((lit<"("> + ref<DEGREE_> +
               assign<&ast::NurbsCurveGeom::degree>(ref<DECIMAL>) + lit<","> +
               assign<&ast::NurbsCurveGeom::controlPoints>(
                   ref<ControlPoints>) +
               lit<","> +
               assign<&ast::NurbsCurveGeom::knots>(ref<Knots>) +
               option(lit<","> + ref<STARTM_> +
                      assign<&ast::NurbsCurveGeom::startM>(ref<DECIMAL>) +
                      lit<","> + ref<ENDM_> +
                      assign<&ast::NurbsCurveGeom::endM>(ref<DECIMAL>)) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct SCALEFACTOR_ : terminal<ikw<"SCALEFACTOR">> {};
  struct STARTDISTANCE_ : terminal<ikw<"STARTDISTANCE">> {};
  struct ENDDISTANCE_ : terminal<ikw<"ENDDISTANCE">> {};

  // ISO/IEC 13249-3 5.1.67's placement-based form, and a (startKappa,
  // endKappa, length) form nested inside a COMPOUNDCURVE: told apart by
  // the token after `(`.
  struct ClothoidGeometry
      : rule<create<ast::ClothoidGeom> + ref<CLOTHOID> + DimClause +
             ((lit<"("> +
               assign<&ast::ClothoidGeom::referenceLocation>(
                   ref<ReferenceLocation>) +
               lit<","> + ref<SCALEFACTOR_> +
               (assign<&ast::ClothoidGeom::scaleFactor>(ref<DECIMAL>) |
                ref<EMPTY_>) +
               lit<","> + ref<STARTDISTANCE_> +
               (assign<&ast::ClothoidGeom::startDistance>(ref<DECIMAL>) |
                ref<EMPTY_>) +
               lit<","> + ref<ENDDISTANCE_> +
               (assign<&ast::ClothoidGeom::endDistance>(ref<DECIMAL>) |
                ref<EMPTY_>) +
               option(lit<","> + ref<STARTM_> +
                      assign<&ast::ClothoidGeom::startM>(ref<DECIMAL>) +
                      lit<","> + ref<ENDM_> +
                      assign<&ast::ClothoidGeom::endM>(ref<DECIMAL>)) +
               lit<")">) |
              (lit<"("> + assign<&ast::ClothoidGeom::startKappa>(Ordinate) +
               lit<","> + assign<&ast::ClothoidGeom::endKappa>(Ordinate) +
               lit<","> + assign<&ast::ClothoidGeom::length>(Ordinate) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct LENGTH_ : terminal<ikw<"LENGTH">> {};
  struct STARTCURVATURE_ : terminal<ikw<"STARTCURVATURE">> {};
  struct ENDCURVATURE_ : terminal<ikw<"ENDCURVATURE">> {};
  struct SPIRALTYPE_ : terminal<ikw<"SPIRALTYPE">> {};

  // Legal only where SPIRALTYPE's value is expected, so per-state lexing
  // alone keeps it from colliding with the geometry keyword vocabulary --
  // no lexer mode needed. Trimmed of trailing trivia in its converter.
  struct SPIRAL_TYPE_NAME : terminal<as<trim_trailing_ws>(cr<R"(^,() \t\r\n)"> +
                                                          many(cr<"^,()">))> {};

  struct SpiralCurveGeometry
      : rule<create<ast::SpiralCurveGeom> + ref<SPIRALCURVE> + DimClause +
             ((lit<"("> +
               assign<&ast::SpiralCurveGeom::referenceLocation>(
                   ref<ReferenceLocation>) +
               lit<","> + ref<LENGTH_> +
               (assign<&ast::SpiralCurveGeom::length>(ref<DECIMAL>) |
                ref<EMPTY_>) +
               lit<","> + ref<STARTCURVATURE_> +
               (assign<&ast::SpiralCurveGeom::startCurvature>(ref<DECIMAL>) |
                ref<EMPTY_>) +
               lit<","> + ref<ENDCURVATURE_> +
               (assign<&ast::SpiralCurveGeom::endCurvature>(ref<DECIMAL>) |
                ref<EMPTY_>) +
               lit<","> + ref<SPIRALTYPE_> +
               assign<&ast::SpiralCurveGeom::spiralType>(
                   ref<SPIRAL_TYPE_NAME>) +
               option(lit<","> + ref<STARTM_> +
                      assign<&ast::SpiralCurveGeom::startM>(ref<DECIMAL>) +
                      lit<","> + ref<ENDM_> +
                      assign<&ast::SpiralCurveGeom::endM>(ref<DECIMAL>)) +
               lit<")">) |
              ref<EMPTY_>)> {};

  // A compound curve's member, and a curve polygon's ring: the same set
  // of shapes either way.
  static constexpr auto CurveMemberChoice =
      ref<PlainPointList> | ref<LineStringGeometry> |
      ref<LinearRingGeometry> | ref<CircularStringGeometry> |
      ref<CircleGeometry> | ref<GeodesicStringGeometry> |
      ref<EllipticalCurveGeometry> | ref<NurbsCurveGeometry> |
      ref<ClothoidGeometry> | ref<SpiralCurveGeometry> |
      ref<CompoundCurveGeometry>;

  struct CompoundCurveGeometry
      : rule<create<ast::CompoundCurveGeom> + ref<COMPOUNDCURVE> + DimClause +
             ((lit<"("> +
               some(append<&ast::CompoundCurveGeom::members>(
                        CurveMemberChoice),
                    lit<",">) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct CurvePolygonGeometry
      : rule<create<ast::CurvePolygonGeom> + ref<CURVEPOLYGON> + DimClause +
             ((lit<"("> +
               some(append<&ast::CurvePolygonGeom::members>(
                        CurveMemberChoice),
                    lit<",">) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct MultiCurveGeometry
      : rule<create<ast::MultiCurveGeom> + ref<MULTICURVE> + DimClause +
             ((lit<"("> +
               some(append<&ast::MultiCurveGeom::members>(CurveMemberChoice),
                    lit<",">) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct PATCHES_ : terminal<ikw<"PATCHES">> {};
  struct ELEMENTS_ : terminal<ikw<"ELEMENTS">> {};
  struct MAXSIDELENGTH_ : terminal<ikw<"MAXSIDELENGTH">> {};
  struct ID_ : terminal<ikw<"ID">> {};
  struct TAG_ : terminal<ikw<"TAG">> {};

  static constexpr auto ElementLabelText =
      option(ref<ID_> + assign<&ast::TinElement::id>(ref<DECIMAL>)) +
      option(ref<TAG_> + assign<&ast::TinElement::tag>(ref<QUOTED_LETTERS>));

  static constexpr auto PatchChoice =
      ref<PolygonGeometry> | ref<TriangleGeometry>;

  struct TinElementTypeText
      : rule<create<ast::TinElement> +
             ((assign<&ast::TinElement::kind>(as<ast::TinElementKind::points>(ikw<"POINTS">)) +
               ElementLabelText +
               assign<&ast::TinElement::payload>(ref<MultiPointGeometry>)) |
              (assign<&ast::TinElement::kind>(as<ast::TinElementKind::groupSpot>(ikw<"GROUPSPOT">)) +
               ElementLabelText +
               assign<&ast::TinElement::payload>(ref<MultiPointGeometry>)) |
              (assign<&ast::TinElement::kind>(as<ast::TinElementKind::boundary>(ikw<"BOUNDARY">)) +
               ElementLabelText +
               assign<&ast::TinElement::payload>(PatchChoice)) |
              (assign<&ast::TinElement::kind>(as<ast::TinElementKind::breakline>(ikw<"BREAKLINE">)) +
               ElementLabelText +
               assign<&ast::TinElement::payload>(ref<LineStringGeometry>)) |
              (assign<&ast::TinElement::kind>(as<ast::TinElementKind::softbreak>(ikw<"SOFTBREAK">)) +
               ElementLabelText +
               assign<&ast::TinElement::payload>(ref<LineStringGeometry>)) |
              (assign<&ast::TinElement::kind>(as<ast::TinElementKind::controlContour>(ikw<"CONTROLCONTOUR">)) +
               ElementLabelText +
               assign<&ast::TinElement::payload>(ref<LineStringGeometry>)) |
              (assign<&ast::TinElement::kind>(as<ast::TinElementKind::breakVoid>(ikw<"BREAKVOID">)) +
               ElementLabelText +
               assign<&ast::TinElement::payload>(PatchChoice)) |
              (assign<&ast::TinElement::kind>(as<ast::TinElementKind::drapeVoid>(ikw<"DRAPEVOID">)) +
               ElementLabelText +
               assign<&ast::TinElement::payload>(PatchChoice)) |
              (assign<&ast::TinElement::kind>(as<ast::TinElementKind::voidElement>(ikw<"VOID">)) + ElementLabelText +
               assign<&ast::TinElement::payload>(PatchChoice)) |
              (assign<&ast::TinElement::kind>(as<ast::TinElementKind::hole>(ikw<"HOLE">)) + ElementLabelText +
               assign<&ast::TinElement::payload>(PatchChoice)) |
              (assign<&ast::TinElement::kind>(as<ast::TinElementKind::stopline>(ikw<"STOPLINE">)) +
               ElementLabelText +
               assign<&ast::TinElement::payload>(
                   ref<LineStringGeometry>)))> {};

  struct PolyhedralSurfaceGeometry
      : rule<create<ast::PolyhedralSurfaceGeom> + ref<POLYHEDRALSURFACE> +
             DimClause +
             ((lit<"("> + ref<PATCHES_> + lit<"("> +
               some(append<&ast::PolyhedralSurfaceGeom::patches>(PatchChoice),
                    lit<",">) +
               lit<")"> + lit<")">) |
              (lit<"("> +
               some(append<&ast::PolyhedralSurfaceGeom::polygons>(
                        ref<BarePolygon>),
                    lit<",">) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct TinGeometry
      : rule<create<ast::TinGeom> + ref<TIN> + DimClause +
             ((lit<"("> + ref<PATCHES_> + lit<"("> +
               some(append<&ast::TinGeom::patchTriangles>(
                        ref<BareTriangle>),
                    lit<",">) +
               lit<")"> +
               option(ref<ELEMENTS_> + lit<"("> +
                      some(append<&ast::TinGeom::elements>(
                               ref<TinElementTypeText>),
                           lit<",">) +
                      lit<")">) +
               option(ref<MAXSIDELENGTH_> +
                      assign<&ast::TinGeom::maxSideLength>(ref<DECIMAL>)) +
               lit<")">) |
              (lit<"("> +
               some(append<&ast::TinGeom::polygons>(ref<BarePolygon>),
                    lit<",">) +
               lit<")">) |
              ref<EMPTY_>)> {};

  // A compound surface's, or a multi-surface's, member.
  static constexpr auto SurfaceMemberChoice =
      ref<PlainPolygon> | ref<PolygonGeometry> | ref<CurvePolygonGeometry> |
      ref<TriangleGeometry> | ref<PolyhedralSurfaceGeometry> |
      ref<TinGeometry> | ref<CompoundSurfaceGeometry>;

  struct CompoundSurfaceGeometry
      : rule<create<ast::CompoundSurfaceGeom> + ref<COMPOUNDSURFACE> +
             DimClause +
             ((lit<"("> +
               some(append<&ast::CompoundSurfaceGeom::members>(
                        SurfaceMemberChoice),
                    lit<",">) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct MultiSurfaceGeometry
      : rule<create<ast::MultiSurfaceGeom> + ref<MULTISURFACE> + DimClause +
             ((lit<"("> +
               some(append<&ast::MultiSurfaceGeom::members>(
                        SurfaceMemberChoice),
                    lit<",">) +
               lit<")">) |
              ref<EMPTY_>)> {};

  static constexpr auto ShellChoice =
      ref<PolyhedralSurfaceGeometry> | ref<CompoundSurfaceGeometry>;

  // The standard mandates a Z dimension for this type.
  struct BrepSolidGeometry
      : rule<create<ast::BrepSolidGeom> + ref<BREPSOLID> +
             assign<&ast::Geometry::dim>(ref<Z_>) +
             ((lit<"("> +
               some(append<&ast::BrepSolidGeom::shells>(ShellChoice),
                    lit<",">) +
               lit<")">) |
              ref<EMPTY_>)> {};

  // The alternatives are grouped as ISO/IEC 13249-3:2016, 5.1.67 groups
  // them: point, curve, surface, solid, then collection.
  static constexpr auto GeometryChoice =
      ref<PointGeometry> | ref<LineStringGeometry> |
      ref<LinearRingGeometry> | ref<CircularStringGeometry> |
      ref<CircleGeometry> | ref<GeodesicStringGeometry> |
      ref<EllipticalCurveGeometry> | ref<NurbsCurveGeometry> |
      ref<ClothoidGeometry> | ref<SpiralCurveGeometry> |
      ref<CompoundCurveGeometry> | ref<PolygonGeometry> |
      ref<TriangleGeometry> | ref<CurvePolygonGeometry> |
      ref<PolyhedralSurfaceGeometry> | ref<TinGeometry> |
      ref<CompoundSurfaceGeometry> | ref<BrepSolidGeometry> |
      ref<MultiPointGeometry> | ref<MultiLineStringGeometry> |
      ref<MultiPolygonGeometry> | ref<MultiCurveGeometry> |
      ref<MultiSurfaceGeometry> | ref<GeometryCollection>;

  struct GeometryCollection
      : rule<create<ast::GeometryCollectionGeom> + ref<GEOMETRYCOLLECTION> +
             DimClause +
             ((lit<"("> +
               some(append<&ast::GeometryCollectionGeom::geometries>(
                        GeometryChoice),
                    lit<",">) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct Document
      : rule<create<ast::Document> +
             many(append<&ast::Document::geometries>(GeometryChoice))> {};

  using entry = Document;
  using ignored = WS;
};

}  // namespace wkt
