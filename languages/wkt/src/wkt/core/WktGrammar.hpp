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
using Unquoted = cursiv::text_conv<unquote>;

// SPIRAL_TYPE_NAME's leading trivia is already gone by the time the token
// starts; only a trailing run before the closing `,`/`)` needs trimming.
inline constexpr auto trim_trailing_ws = [](std::string_view t) {
  while (!t.empty() && (t.back() == ' ' || t.back() == '\t' ||
                        t.back() == '\r' || t.back() == '\n'))
    t.remove_suffix(1);
  return t;
};
using Trimmed = cursiv::text_conv<trim_trailing_ws>;

struct Wkt : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  struct DECIMAL
      : terminal<option(cr<"+-">) + ((some(d) + option(lit<"."> + many(d))) |
                                      (lit<"."> + some(d))) +
                     option(cr<"eE"> + option(cr<"+-">) + some(d)),
                 double> {};
  inline static constexpr auto special_value = [](std::string_view text) {
    return *ordinate_value(text);
  };
  struct SPECIAL_NUMBER
      : terminal<option(cr<"+-">) + (ikw<"NAN"> | ikw<"INF"> | ikw<"INFINITY">),
                 text_conv<special_value>> {};
  static constexpr auto Ordinate = ref<DECIMAL> | ref<SPECIAL_NUMBER>;
  struct NUMBER : terminal<some(cr<R"(^ \t\r\n(),)">)> {};

  template<cursiv::str Text>
  static constexpr auto GeometryKeyword = cursiv::Lit<Text, true>{} +
      &(option(ikw<"ZM"> | ikw<"Z"> | ikw<"M">) + !w);

  struct EMPTY_ : terminal<ikw<"EMPTY">> {};
  struct ZM_ : terminal<ikw<"ZM">, value<ast::Dim::zm>> {};
  struct Z_ : terminal<ikw<"Z">, value<ast::Dim::z>> {};
  struct M_ : terminal<ikw<"M">, value<ast::Dim::m>> {};
  static constexpr auto DimSpec = ref<ZM_> | ref<Z_> | ref<M_>;
  static constexpr auto DimClause = option(assign<&ast::Geometry::dim>(DimSpec));

  struct QUOTED_LETTERS
      : terminal<lit<"\""> + some(cr<"a-zA-Z0-9()_.' -">) + lit<"\"">,
                 Unquoted> {};

  // Every geometry-producing rule is forward declared once, up front, so the
  // choices below (a curve member nesting COMPOUNDCURVE, a surface member
  // nesting COMPOUNDSURFACE, GEOMETRYCOLLECTION nesting `geometry` itself)
  // can name each other regardless of definition order.
  struct PointGeometryRule;
  struct LineStringGeometryRule;
  struct LinearRingGeometryRule;
  struct CircularStringGeometryRule;
  struct CircleGeometryRule;
  struct GeodesicStringGeometryRule;
  struct EllipticalCurveGeometryRule;
  struct NurbsCurveGeometryRule;
  struct ClothoidGeometryRule;
  struct SpiralCurveGeometryRule;
  struct CompoundCurveGeometryRule;
  struct PolygonGeometryRule;
  struct TriangleGeometryRule;
  struct CurvePolygonGeometryRule;
  struct PolyhedralSurfaceGeometryRule;
  struct TinGeometryRule;
  struct CompoundSurfaceGeometryRule;
  struct BrepSolidGeometryRule;
  struct MultiPointGeometryRule;
  struct MultiLineStringGeometryRule;
  struct MultiPolygonGeometryRule;
  struct MultiCurveGeometryRule;
  struct MultiSurfaceGeometryRule;
  struct GeometryCollectionRule;

  struct PointRule
      : rule<create<ast::Point> +
             many(append<&ast::Point::ordinates>(ref<NUMBER>))> {};

  struct VectorTupleRule : rule<ref<PointRule>> {};

  // A bare "(p, p, ...)" run, used wherever the standard has no keyword of
  // its own: a polygon ring, a multi-line-string's member, a bare curve
  // segment. Zero points means the run came from EMPTY.
  static constexpr auto PointListBodyText =
      (lit<"("> +
       some(append<&ast::PointList::points>(ref<PointRule>), lit<",">) +
       lit<")">) |
      ref<EMPTY_>;
  struct PointListRule : rule<create<ast::PointList> +
                              option(assign<&ast::PointList::dim>(DimSpec)) +
                              PointListBodyText> {};
  struct PlainPointListRule : rule<create<ast::PointList> + PointListBodyText> {
  };

  static constexpr auto BarePolygonBody =
      (lit<"("> +
       some(append<&ast::BarePolygon::rings>(ref<PointListRule>), lit<",">) +
       lit<")">) |
      ref<EMPTY_>;
  struct BarePolygonRule
      : rule<create<ast::BarePolygon> +
             option(assign<&ast::BarePolygon::dim>(DimSpec)) +
             BarePolygonBody> {};
  struct PlainPolygonRule : rule<create<ast::BarePolygon> + BarePolygonBody> {};

  struct MultiPointItemRule
      : rule<create<ast::MultiPointItem> +
             option(assign<&ast::MultiPointItem::dim>(DimSpec)) +
             (assign<&ast::MultiPointItem::value>(ref<PointRule>) |
              (lit<"("> + assign<&ast::MultiPointItem::value>(ref<PointRule>) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct LOCATION_ : terminal<ikw<"LOCATION">> {};
  struct AFFINEPLACEMENT_ : terminal<ikw<"AFFINEPLACEMENT">> {};
  struct REFERENCEDIRECTIONS_ : terminal<ikw<"REFERENCEDIRECTIONS">> {};
  struct VECTOR_ : terminal<ikw<"VECTOR">> {};
  struct REFERENCELOCATION_ : terminal<ikw<"REFERENCELOCATION">> {};

  struct LocationRule
      : rule<create<ast::Location> + ref<LOCATION_> +
             option(flag<&ast::Location::hasZ>(ref<Z_>)) +
             ((lit<"("> + assign<&ast::Location::value>(ref<PointRule>) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct VectorEntryRule
      : rule<create<ast::Vector> + ref<VECTOR_> +
             option(flag<&ast::Vector::hasZ>(ref<Z_>)) +
             ((lit<"("> + assign<&ast::Vector::value>(ref<VectorTupleRule>) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct ReferenceDirectionsRule
      : rule<create<ast::ReferenceDirections> + ref<REFERENCEDIRECTIONS_> +
             ((lit<"("> +
               some(append<&ast::ReferenceDirections::vectors>(
                        ref<VectorEntryRule>),
                    lit<",">) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct AffinePlacementRule
      : rule<create<ast::AffinePlacement> + ref<AFFINEPLACEMENT_> +
             option(flag<&ast::AffinePlacement::hasZ>(ref<Z_>)) +
             ((lit<"("> +
               assign<&ast::AffinePlacement::location>(ref<LocationRule>) +
               lit<","> +
               assign<&ast::AffinePlacement::directions>(
                   ref<ReferenceDirectionsRule>) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct ReferenceLocationRule
      : rule<create<ast::ReferenceLocation> + ref<REFERENCELOCATION_> +
             assign<&ast::ReferenceLocation::placement>(
                 ref<AffinePlacementRule>)> {};

  struct DEGREES_ : terminal<ikw<"DEGREES">, value<ast::AngleUnit::degrees>> {};
  struct GRADIANS_
      : terminal<ikw<"GRADIANS">, value<ast::AngleUnit::gradians>> {};
  struct RADIANS_ : terminal<ikw<"RADIANS">, value<ast::AngleUnit::radians>> {};

  struct AngleRule
      : rule<create<ast::Angle> +
             assign<&ast::Angle::unit>(ref<DEGREES_> | ref<GRADIANS_> |
                                       ref<RADIANS_>) +
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

  struct PointGeometryRule
      : rule<create<ast::PointGeom> + ref<POINT> + DimClause +
             ((lit<"("> + assign<&ast::PointGeom::value>(ref<PointRule>) +
               lit<")">) |
              ref<EMPTY_>)> {};

  static constexpr auto PointListBody =
      (lit<"("> +
       some(append<&ast::PointListGeometry::points>(ref<PointRule>), lit<",">) +
       lit<")">) |
      ref<EMPTY_>;

  struct LineStringGeometryRule
      : rule<create<ast::LineStringGeom> + ref<LINESTRING> + DimClause +
             PointListBody> {};
  struct LinearRingGeometryRule
      : rule<create<ast::LinearRingGeom> + ref<LINEARRING> + DimClause +
             PointListBody> {};
  struct CircularStringGeometryRule
      : rule<create<ast::CircularStringGeom> + ref<CIRCULARSTRING> +
             DimClause + PointListBody> {};
  struct CircleGeometryRule
      : rule<create<ast::CircleGeom> + ref<CIRCLE> + DimClause +
             PointListBody> {};
  struct GeodesicStringGeometryRule
      : rule<create<ast::GeodesicStringGeom> + ref<GEODESICSTRING> +
             DimClause + PointListBody> {};

  struct PolygonGeometryRule
      : rule<create<ast::PolygonGeom> + ref<POLYGON> + DimClause +
             ((lit<"("> +
               some(append<&ast::PolygonGeom::rings>(ref<PointListRule>),
                    lit<",">) +
               lit<")">) |
              ref<EMPTY_>)> {};

  static constexpr auto TriangleBody =
      (lit<"("> + append<&ast::TriangleGeom::corners>(ref<PointRule>) +
       lit<","> + append<&ast::TriangleGeom::corners>(ref<PointRule>) +
       lit<","> + append<&ast::TriangleGeom::corners>(ref<PointRule>) +
       lit<")">) |
      (lit<"("> +
       some(append<&ast::TriangleGeom::rings>(ref<PointListRule>), lit<",">) +
       lit<")">) |
      ref<EMPTY_>;

  struct TriangleGeometryRule
      : rule<create<ast::TriangleGeom> + ref<TRIANGLE> + DimClause +
             TriangleBody> {};
  // A TIN's PATCHES list: the same body as TriangleGeometryRule, no
  // TRIANGLE keyword.
  struct BareTriangleRule : rule<create<ast::TriangleGeom> + TriangleBody> {};

  struct MultiPointGeometryRule
      : rule<create<ast::MultiPointGeom> + ref<MULTIPOINT> + DimClause +
             ((lit<"("> +
               some(append<&ast::MultiPointGeom::items>(
                        ref<MultiPointItemRule>),
                    lit<",">) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct MultiLineStringGeometryRule
      : rule<create<ast::MultiLineStringGeom> + ref<MULTILINESTRING> +
             DimClause +
             ((lit<"("> +
               some(append<&ast::MultiLineStringGeom::lines>(
                        ref<PointListRule>),
                    lit<",">) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct MultiPolygonGeometryRule
      : rule<create<ast::MultiPolygonGeom> + ref<MULTIPOLYGON> + DimClause +
             ((lit<"("> +
               some(append<&ast::MultiPolygonGeom::polygons>(
                        ref<BarePolygonRule>),
                    lit<",">) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct UAXISLENGTH_ : terminal<ikw<"UAXISLENGTH">> {};
  struct VAXISLENGTH_ : terminal<ikw<"VAXISLENGTH">> {};
  struct STARTANGLE_ : terminal<ikw<"STARTANGLE">> {};
  struct ENDANGLE_ : terminal<ikw<"ENDANGLE">> {};

  struct EllipticalCurveGeometryRule
      : rule<create<ast::EllipticalCurveGeom> + ref<ELLIPTICALCURVE> +
             DimClause +
             ((lit<"("> +
               assign<&ast::EllipticalCurveGeom::referenceLocation>(
                   ref<ReferenceLocationRule>) +
               lit<","> + ref<UAXISLENGTH_> +
               (assign<&ast::EllipticalCurveGeom::uAxisLength>(ref<DECIMAL>) |
                ref<EMPTY_>) +
               lit<","> + ref<VAXISLENGTH_> +
               (assign<&ast::EllipticalCurveGeom::vAxisLength>(ref<DECIMAL>) |
                ref<EMPTY_>) +
               lit<","> + ref<STARTANGLE_> +
               assign<&ast::EllipticalCurveGeom::startAngle>(ref<AngleRule>) +
               lit<","> + ref<ENDANGLE_> +
               assign<&ast::EllipticalCurveGeom::endAngle>(ref<AngleRule>) +
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

  struct WeightedPointRule
      : rule<create<ast::WeightedPoint> + ref<WEIGHTEDPOINT_> +
             option(flag<&ast::WeightedPoint::hasZ>(ref<Z_>)) +
             ((lit<"("> + assign<&ast::WeightedPoint::value>(ref<PointRule>) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct NurbsPointRule
      : rule<create<ast::NurbsPoint> + ref<NURBSPOINT_> +
             ((lit<"("> +
               assign<&ast::NurbsPoint::point>(ref<WeightedPointRule>) +
               lit<","> + ref<WEIGHT_> +
               assign<&ast::NurbsPoint::weight>(ref<DECIMAL>) + lit<")">) |
              ref<EMPTY_>)> {};

  struct ControlPointsRule
      : rule<create<ast::ControlPoints> + ref<CONTROLPOINTS_> +
             option(flag<&ast::ControlPoints::hasZ>(ref<Z_>)) +
             ((lit<"("> +
               some(append<&ast::ControlPoints::points>(ref<NurbsPointRule>),
                    lit<",">) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct KnotRule
      : rule<create<ast::Knot> + ref<KNOT_> +
             ((lit<"("> + ref<VALUE_> +
               assign<&ast::Knot::value>(ref<DECIMAL>) + lit<","> +
               ref<MULTIPLICITY_> +
               assign<&ast::Knot::multiplicity>(ref<DECIMAL>) + lit<")">) |
              ref<EMPTY_>)> {};

  struct KnotsRule
      : rule<create<ast::Knots> + ref<KNOTS_> +
             ((lit<"("> +
               some(append<&ast::Knots::knots>(ref<KnotRule>), lit<",">) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct NurbsCurveGeometryRule
      : rule<create<ast::NurbsCurveGeom> + ref<NURBSCURVE> + DimClause +
             ((lit<"("> + ref<DEGREE_> +
               assign<&ast::NurbsCurveGeom::degree>(ref<DECIMAL>) + lit<","> +
               assign<&ast::NurbsCurveGeom::controlPoints>(
                   ref<ControlPointsRule>) +
               lit<","> +
               assign<&ast::NurbsCurveGeom::knots>(ref<KnotsRule>) +
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
  struct ClothoidGeometryRule
      : rule<create<ast::ClothoidGeom> + ref<CLOTHOID> + DimClause +
             ((lit<"("> +
               assign<&ast::ClothoidGeom::referenceLocation>(
                   ref<ReferenceLocationRule>) +
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
  struct SPIRAL_TYPE_NAME
      : terminal<cr<R"(^,() \t\r\n)"> + many(cr<"^,()">), Trimmed> {};

  struct SpiralCurveGeometryRule
      : rule<create<ast::SpiralCurveGeom> + ref<SPIRALCURVE> + DimClause +
             ((lit<"("> +
               assign<&ast::SpiralCurveGeom::referenceLocation>(
                   ref<ReferenceLocationRule>) +
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
      ref<PlainPointListRule> | ref<LineStringGeometryRule> |
      ref<LinearRingGeometryRule> | ref<CircularStringGeometryRule> |
      ref<CircleGeometryRule> | ref<GeodesicStringGeometryRule> |
      ref<EllipticalCurveGeometryRule> | ref<NurbsCurveGeometryRule> |
      ref<ClothoidGeometryRule> | ref<SpiralCurveGeometryRule> |
      ref<CompoundCurveGeometryRule>;

  struct CompoundCurveGeometryRule
      : rule<create<ast::CompoundCurveGeom> + ref<COMPOUNDCURVE> + DimClause +
             ((lit<"("> +
               some(append<&ast::CompoundCurveGeom::members>(
                        CurveMemberChoice),
                    lit<",">) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct CurvePolygonGeometryRule
      : rule<create<ast::CurvePolygonGeom> + ref<CURVEPOLYGON> + DimClause +
             ((lit<"("> +
               some(append<&ast::CurvePolygonGeom::members>(
                        CurveMemberChoice),
                    lit<",">) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct MultiCurveGeometryRule
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
  struct POINTS_ : terminal<ikw<"POINTS">, value<ast::TinElementKind::points>> {
  };
  struct GROUPSPOT_
      : terminal<ikw<"GROUPSPOT">, value<ast::TinElementKind::groupSpot>> {};
  struct BOUNDARY_
      : terminal<ikw<"BOUNDARY">, value<ast::TinElementKind::boundary>> {};
  struct BREAKLINE_
      : terminal<ikw<"BREAKLINE">, value<ast::TinElementKind::breakline>> {};
  struct SOFTBREAK_
      : terminal<ikw<"SOFTBREAK">, value<ast::TinElementKind::softbreak>> {};
  struct CONTROLCONTOUR_
      : terminal<ikw<"CONTROLCONTOUR">,
                 value<ast::TinElementKind::controlContour>> {};
  struct BREAKVOID_
      : terminal<ikw<"BREAKVOID">, value<ast::TinElementKind::breakVoid>> {};
  struct DRAPEVOID_
      : terminal<ikw<"DRAPEVOID">, value<ast::TinElementKind::drapeVoid>> {};
  struct VOID_
      : terminal<ikw<"VOID">, value<ast::TinElementKind::voidElement>> {};
  struct HOLE_ : terminal<ikw<"HOLE">, value<ast::TinElementKind::hole>> {};
  struct STOPLINE_
      : terminal<ikw<"STOPLINE">, value<ast::TinElementKind::stopline>> {};

  static constexpr auto ElementLabelText =
      option(ref<ID_> + assign<&ast::TinElement::id>(ref<DECIMAL>)) +
      option(ref<TAG_> + assign<&ast::TinElement::tag>(ref<QUOTED_LETTERS>));

  static constexpr auto PatchChoice =
      ref<PolygonGeometryRule> | ref<TriangleGeometryRule>;

  struct TinElementTypeTextRule
      : rule<create<ast::TinElement> +
             ((assign<&ast::TinElement::kind>(ref<POINTS_>) +
               ElementLabelText +
               assign<&ast::TinElement::payload>(ref<MultiPointGeometryRule>)) |
              (assign<&ast::TinElement::kind>(ref<GROUPSPOT_>) +
               ElementLabelText +
               assign<&ast::TinElement::payload>(ref<MultiPointGeometryRule>)) |
              (assign<&ast::TinElement::kind>(ref<BOUNDARY_>) +
               ElementLabelText +
               assign<&ast::TinElement::payload>(PatchChoice)) |
              (assign<&ast::TinElement::kind>(ref<BREAKLINE_>) +
               ElementLabelText +
               assign<&ast::TinElement::payload>(ref<LineStringGeometryRule>)) |
              (assign<&ast::TinElement::kind>(ref<SOFTBREAK_>) +
               ElementLabelText +
               assign<&ast::TinElement::payload>(ref<LineStringGeometryRule>)) |
              (assign<&ast::TinElement::kind>(ref<CONTROLCONTOUR_>) +
               ElementLabelText +
               assign<&ast::TinElement::payload>(ref<LineStringGeometryRule>)) |
              (assign<&ast::TinElement::kind>(ref<BREAKVOID_>) +
               ElementLabelText +
               assign<&ast::TinElement::payload>(PatchChoice)) |
              (assign<&ast::TinElement::kind>(ref<DRAPEVOID_>) +
               ElementLabelText +
               assign<&ast::TinElement::payload>(PatchChoice)) |
              (assign<&ast::TinElement::kind>(ref<VOID_>) + ElementLabelText +
               assign<&ast::TinElement::payload>(PatchChoice)) |
              (assign<&ast::TinElement::kind>(ref<HOLE_>) + ElementLabelText +
               assign<&ast::TinElement::payload>(PatchChoice)) |
              (assign<&ast::TinElement::kind>(ref<STOPLINE_>) +
               ElementLabelText +
               assign<&ast::TinElement::payload>(
                   ref<LineStringGeometryRule>)))> {};

  struct PolyhedralSurfaceGeometryRule
      : rule<create<ast::PolyhedralSurfaceGeom> + ref<POLYHEDRALSURFACE> +
             DimClause +
             ((lit<"("> + ref<PATCHES_> + lit<"("> +
               some(append<&ast::PolyhedralSurfaceGeom::patches>(PatchChoice),
                    lit<",">) +
               lit<")"> + lit<")">) |
              (lit<"("> +
               some(append<&ast::PolyhedralSurfaceGeom::polygons>(
                        ref<BarePolygonRule>),
                    lit<",">) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct TinGeometryRule
      : rule<create<ast::TinGeom> + ref<TIN> + DimClause +
             ((lit<"("> + ref<PATCHES_> + lit<"("> +
               some(append<&ast::TinGeom::patchTriangles>(
                        ref<BareTriangleRule>),
                    lit<",">) +
               lit<")"> +
               option(ref<ELEMENTS_> + lit<"("> +
                      some(append<&ast::TinGeom::elements>(
                               ref<TinElementTypeTextRule>),
                           lit<",">) +
                      lit<")">) +
               option(ref<MAXSIDELENGTH_> +
                      assign<&ast::TinGeom::maxSideLength>(ref<DECIMAL>)) +
               lit<")">) |
              (lit<"("> +
               some(append<&ast::TinGeom::polygons>(ref<BarePolygonRule>),
                    lit<",">) +
               lit<")">) |
              ref<EMPTY_>)> {};

  // A compound surface's, or a multi-surface's, member.
  static constexpr auto SurfaceMemberChoice =
      ref<PlainPolygonRule> | ref<PolygonGeometryRule> | ref<CurvePolygonGeometryRule> |
      ref<TriangleGeometryRule> | ref<PolyhedralSurfaceGeometryRule> |
      ref<TinGeometryRule> | ref<CompoundSurfaceGeometryRule>;

  struct CompoundSurfaceGeometryRule
      : rule<create<ast::CompoundSurfaceGeom> + ref<COMPOUNDSURFACE> +
             DimClause +
             ((lit<"("> +
               some(append<&ast::CompoundSurfaceGeom::members>(
                        SurfaceMemberChoice),
                    lit<",">) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct MultiSurfaceGeometryRule
      : rule<create<ast::MultiSurfaceGeom> + ref<MULTISURFACE> + DimClause +
             ((lit<"("> +
               some(append<&ast::MultiSurfaceGeom::members>(
                        SurfaceMemberChoice),
                    lit<",">) +
               lit<")">) |
              ref<EMPTY_>)> {};

  static constexpr auto ShellChoice =
      ref<PolyhedralSurfaceGeometryRule> | ref<CompoundSurfaceGeometryRule>;

  // The standard mandates a Z dimension for this type.
  struct BrepSolidGeometryRule
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
      ref<PointGeometryRule> | ref<LineStringGeometryRule> |
      ref<LinearRingGeometryRule> | ref<CircularStringGeometryRule> |
      ref<CircleGeometryRule> | ref<GeodesicStringGeometryRule> |
      ref<EllipticalCurveGeometryRule> | ref<NurbsCurveGeometryRule> |
      ref<ClothoidGeometryRule> | ref<SpiralCurveGeometryRule> |
      ref<CompoundCurveGeometryRule> | ref<PolygonGeometryRule> |
      ref<TriangleGeometryRule> | ref<CurvePolygonGeometryRule> |
      ref<PolyhedralSurfaceGeometryRule> | ref<TinGeometryRule> |
      ref<CompoundSurfaceGeometryRule> | ref<BrepSolidGeometryRule> |
      ref<MultiPointGeometryRule> | ref<MultiLineStringGeometryRule> |
      ref<MultiPolygonGeometryRule> | ref<MultiCurveGeometryRule> |
      ref<MultiSurfaceGeometryRule> | ref<GeometryCollectionRule>;

  struct GeometryCollectionRule
      : rule<create<ast::GeometryCollectionGeom> + ref<GEOMETRYCOLLECTION> +
             DimClause +
             ((lit<"("> +
               some(append<&ast::GeometryCollectionGeom::geometries>(
                        GeometryChoice),
                    lit<",">) +
               lit<")">) |
              ref<EMPTY_>)> {};

  struct DocumentRule
      : rule<create<ast::Document> +
             many(append<&ast::Document::geometries>(GeometryChoice))> {};

  using entry = DocumentRule;
  using ignore = WS;
};

}  // namespace wkt
