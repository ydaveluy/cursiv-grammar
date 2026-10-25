#pragma once

#include <cmath>
#include <string>
#include <vector>

#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "wkt/core/CoreServices.hpp"
#include "wkt/core/Numbers.hpp"

inline bool native_regressions() {
  namespace ast = wkt::ast;
  auto language =
      cursiv::create_language<wkt::WktCoreServices, wkt::WktSharedCoreServices,
                              wkt::install_core_module>();
  const auto clean = [](const auto& r) {
    return r.accepted && !r.cst().recovered() && r.diagnostics.empty() && r.value;
  };
  for (const std::string type :
       {"POINT", "LINESTRING", "LINEARRING", "POLYGON", "MULTIPOINT", "MULTILINESTRING",
        "MULTIPOLYGON", "GEOMETRYCOLLECTION", "CIRCULARSTRING", "COMPOUNDCURVE",
        "CURVEPOLYGON", "MULTICURVE", "MULTISURFACE"}) {
    for (const auto& [suffix, dim] :
         std::vector<std::pair<std::string, ast::Dim>>{{"", ast::Dim::none},
                                                       {"Z", ast::Dim::z},
                                                       {"M", ast::Dim::m},
                                                       {"ZM", ast::Dim::zm}}) {
      const auto source = type + suffix + " EMPTY";
      const auto r = language.language.parser->parse(source);
      if (!clean(r)) return false;
      const auto* doc = cursiv::ast_cast<ast::Document>(r.value);
      if (!doc || doc->geometries.size() != 1 || doc->geometries[0]->dim != dim ||
          doc->geometries[0]->parent != doc)
        return false;
    }
  }
  const auto r = language.language.parser->parse("POINTZM(1e999 -1e-999 0x1p+3 +.25)");
  if (!clean(r)) return false;
  const auto* g = cursiv::ast_cast<ast::PointGeom>(
      cursiv::ast_cast<ast::Document>(r.value)->geometries[0]);
  if (!g || !g->value || g->value->ordinates.size() != 4 ||
      g->value->ordinates[0] != "1e999" || g->value->ordinates[1] != "-1e-999" ||
      g->value->ordinates[2] != "0x1p+3" || g->value->ordinates[3] != "+.25" ||
      *wkt::ordinate_value(g->value->ordinates[0]) != INFINITY ||
      !std::signbit(*wkt::ordinate_value(g->value->ordinates[1])) ||
      *wkt::ordinate_value(g->value->ordinates[2]) != 8 ||
      *wkt::ordinate_value(g->value->ordinates[3]) != .25)
    return false;
  const auto typed = language.language.parser->parse(
      "COMPOUNDCURVE(LINESTRING(0 0,1 1),CIRCULARSTRING(1 1,2 2,3 1))");
  if (!clean(typed)) return false;
  const auto* compound = cursiv::ast_cast<ast::CompoundCurveGeom>(
      cursiv::ast_cast<ast::Document>(typed.value)->geometries[0]);
  if (!compound || compound->members.size() != 2) return false;
  const auto* first = std::get_if<ast::Geometry*>(&compound->members[0]);
  if (!first || !cursiv::ast_cast<ast::LineStringGeom>(*first) ||
      (*first)->parent != compound)
    return false;
  for (const std::string source :
       {"POINT (Infinity -Infinity)", "LINESTRING(+NaN -NaN,0x1p2 1e999)",
        "LINEARRING(0 0,1 1,0 0)", "CIRCULARSTRING(0 0)",
        "CIRCULARSTRING(0 0,1 1,2 2,3 3)",
        "MULTISURFACE(POLYGON((0 0,1 0,1 1,0 0)),EMPTY)"}) {
    const auto p = language.language.parser->parse(source);
    if (!clean(p) ||
        !cursiv::test::validationHelper(language.language, source).diagnostics.empty())
      return false;
  }
  struct Invalid {
    std::string source;
    std::string code;
  };
  for (const auto& c : std::vector<Invalid>{
           {"POINT(1)", "wkt.coordinate-arity"},
           {"POINT(1 2 3 4 5)", "wkt.coordinate-arity"},
           {"POINT()", "wkt.coordinate-arity"},
           {"POINT(1-2 3)", "wkt.ordinate"},
           {"POINT(1e 2)", "wkt.ordinate"},
           {"POINT Z(1 2)", "wkt.dimension"},
           {"LINESTRING(0 0,1 1 2)", "wkt.dimension"},
           {"LINESTRING(1 2)", "wkt.line-size"},
           {"POLYGON((0 0,1 0,1 1))", "wkt.ring-closure"},
           {"LINEARRING(0 0,0 0)", "wkt.ring-size"},
           {"CIRCULARSTRING(0 0,1 1)", "wkt.circular-size"},
           {"GEOMETRYCOLLECTION M(POINTZ(1 2 3))", "wkt.child-dimension"},
           {"COMPOUNDCURVE(EMPTY)", "wkt.empty-curve-section"},
           {"COMPOUNDCURVE((0 0,1 1),(2 2,3 3))", "wkt.curve-continuity"},
           {"POLYGON(EMPTY,(0 0,1 0,1 1,0 0))", "wkt.empty-shell"}}) {
    const auto p = language.language.parser->parse(c.source);
    if (!clean(p)) return false;
    const auto v = cursiv::test::validationHelper(language.language, c.source);
    if (v.diagnostics.size() != 1 || !v.diagnostics[0].code ||
        v.diagnostics[0].code->string() != c.code)
      return false;
  }
  const auto location = cursiv::test::validationHelper(language.language, "POINT(1e 2)");
  if (location.diagnostics.size() != 1 || location.diagnostics[0].range.begin != 6 ||
      location.diagnostics[0].range.end != 8) return false;
  return true;
}
