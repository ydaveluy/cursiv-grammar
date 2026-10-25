#pragma once
#include "cursiv/core/services/JsonValue.hpp"
#include "wkt/core/Dimensions.hpp"
#include "wkt/core/Numbers.hpp"
#include <algorithm>
#include <cmath>
namespace wkt_test {
namespace ast = wkt::ast;
using J = cursiv::JsonValue;
inline J number(double value) {
  if (std::isnan(value))
    return "NaN";
  if (std::isinf(value))
    return value < 0 ? "-Infinity" : "Infinity";
  return value;
}
inline J sequence(const auto &points, ast::Dim dim, const char *type) {
  J::Array coordinates;
  for (const auto *point : points) {
    J::Array values;
    for (const auto raw : point->ordinates) {
      const auto value = wkt::ordinate_value(raw);
      values.push_back(value ? number(*value) : J(nullptr));
    }
    coordinates.push_back(std::move(values));
  }
  return J::Object{{"type", type},
                   {"z", bool(static_cast<unsigned>(dim) & 1)},
                   {"m", bool(static_cast<unsigned>(dim) & 2)},
                   {"empty", coordinates.empty()},
                   {"coordinates", coordinates}};
}
inline J grouped(const char *type, ast::Dim dim, J::Array children,
                 const char *field = "geometries") {
  const bool empty =
      std::all_of(children.begin(), children.end(), [](const J &child) {
        return child.object().at("empty").boolean();
      });
  return J::Object{{"type", type},
                   {"z", bool(static_cast<unsigned>(dim) & 1)},
                   {"m", bool(static_cast<unsigned>(dim) & 2)},
                   {"empty", empty},
                   {field, std::move(children)}};
}
inline J polygon(const auto &rings, ast::Dim dim,
                 const wkt::Dimensions &dimensions) {
  J::Array children;
  for (const auto *ring : rings)
    children.push_back(
        sequence(ring->points, dimensions.get(*ring), "LinearRing"));
  J result = grouped("Polygon", dim, std::move(children), "rings");
  if (result.object().at("empty").boolean())
    result.object()["rings"] = J::Array{};
  return result;
}
inline J geometry(const ast::Geometry &g, const wkt::Dimensions &dimensions) {
  const auto dim = dimensions.get(g);
  if (const auto *p = cursiv::ast_cast<ast::PointGeom>(&g)) {
    std::vector<ast::Point *> points;
    if (p->value)
      points.push_back(p->value);
    return sequence(points, dim, "Point");
  }
  if (const auto *p = cursiv::ast_cast<ast::PointListGeometry>(&g)) {
    const auto *kind = cursiv::ast_cast<ast::LineStringGeom>(&g) ? "LineString"
                       : cursiv::ast_cast<ast::LinearRingGeom>(&g)
                           ? "LinearRing"
                           : "CircularString";
    return sequence(p->points, dim, kind);
  }
  if (const auto *p = cursiv::ast_cast<ast::PolygonGeom>(&g))
    return polygon(p->rings, dim, dimensions);
  if (const auto *p = cursiv::ast_cast<ast::MultiPointGeom>(&g)) {
    J::Array children;
    for (const auto *item : p->items) {
      std::vector<ast::Point *> points;
      if (item->value)
        points.push_back(item->value);
      children.push_back(sequence(points, dimensions.get(*item), "Point"));
    }
    return grouped("MultiPoint", dim, std::move(children));
  }
  if (const auto *p = cursiv::ast_cast<ast::MultiLineStringGeom>(&g)) {
    J::Array children;
    for (const auto *line : p->lines)
      children.push_back(
          sequence(line->points, dimensions.get(*line), "LineString"));
    return grouped("MultiLineString", dim, std::move(children));
  }
  if (const auto *p = cursiv::ast_cast<ast::MultiPolygonGeom>(&g)) {
    J::Array children;
    for (const auto *poly : p->polygons)
      children.push_back(
          polygon(poly->rings, dimensions.get(*poly), dimensions));
    return grouped("MultiPolygon", dim, std::move(children));
  }
  if (const auto *p = cursiv::ast_cast<ast::GeometryCollectionGeom>(&g)) {
    J::Array children;
    for (const auto *child : p->geometries)
      children.push_back(geometry(*child, dimensions));
    return grouped("GeometryCollection", dim, std::move(children));
  }
  const auto curves = [&](const auto &members, const char *type,
                          const char *field) {
    J::Array children;
    for (const auto &member : members) {
      if (const auto *line = std::get_if<ast::PointList *>(&member))
        children.push_back(
            sequence((*line)->points, dimensions.get(**line), "LineString"));
      else
        children.push_back(
            geometry(*std::get<ast::Geometry *>(member), dimensions));
    }
    J result = grouped(type, dim, std::move(children), field);
    if (std::string_view(field) == "rings" &&
        result.object().at("empty").boolean())
      result.object()["rings"] = J::Array{};
    return result;
  };
  if (const auto *p = cursiv::ast_cast<ast::CompoundCurveGeom>(&g))
    return curves(p->members, "CompoundCurve", "curves");
  if (const auto *p = cursiv::ast_cast<ast::MultiCurveGeom>(&g))
    return curves(p->members, "MultiCurve", "geometries");
  if (const auto *p = cursiv::ast_cast<ast::CurvePolygonGeom>(&g))
    return curves(p->members, "CurvePolygon", "rings");
  if (const auto *p = cursiv::ast_cast<ast::MultiSurfaceGeom>(&g)) {
    J::Array children;
    for (const auto &member : p->members) {
      if (const auto *poly = std::get_if<ast::BarePolygon *>(&member))
        children.push_back(
            polygon((*poly)->rings, dimensions.get(**poly), dimensions));
      else
        children.push_back(
            geometry(*std::get<ast::Geometry *>(member), dimensions));
    }
    return grouped("MultiSurface", dim, std::move(children));
  }
  return nullptr;
}
} // namespace wkt_test
