#pragma once

#include <unordered_map>
#include <vector>

#include "wkt/core/ast.hpp"

namespace wkt {
class Dimensions {
public:
  enum class Error { marker, coordinate, child };
  struct Issue {
    const cursiv::AstNode *node;
    Error error;
  };

  explicit Dimensions(const ast::Document &document) {
    for (const auto *geometry : document.geometries)
      if (geometry)
        visit(*geometry, {});
  }
  ast::Dim get(const cursiv::AstNode &node) const {
    const auto it = values.find(&node);
    return it == values.end() ? ast::Dim::none : it->second;
  }
  const std::vector<Issue> &issues() const { return errors; }

private:
  struct State {
    ast::Dim dim = ast::Dim::none;
    bool fixed = false;
  };
  static ast::Dim combine(ast::Dim a, ast::Dim b) {
    return static_cast<ast::Dim>(static_cast<unsigned>(a) |
                                 static_cast<unsigned>(b));
  }
  void marker(const cursiv::AstNode &node, ast::Dim dim, State &state) {
    if (dim == ast::Dim::none)
      return;
    if (state.fixed)
      errors.push_back({&node, Error::marker});
    else
      state = {dim, true};
  }
  void point(const ast::Point &point, State &state) {
    const auto count = point.ordinates.size();
    if (!state.fixed && count >= 2 && count <= 4)
      state = {count == 4   ? ast::Dim::zm
               : count == 3 ? ast::Dim::z
                            : ast::Dim::none,
               true};
    const auto expected = state.dim == ast::Dim::zm     ? 4u
                          : state.dim == ast::Dim::none ? 2u
                                                        : 3u;
    if (count >= 2 && count <= 4 && count != expected)
      errors.push_back({&point, Error::coordinate});
    values[&point] = state.dim;
  }
  void points(const auto &points, State &state) {
    for (const auto *p : points)
      if (p)
        point(*p, state);
  }
  void list(const ast::PointList &list, State &state) {
    marker(list, list.dim, state);
    points(list.points, state);
    values[&list] = state.dim;
  }
  void polygon(const ast::BarePolygon &polygon, State &state) {
    marker(polygon, polygon.dim, state);
    for (const auto *ring : polygon.rings)
      if (ring)
        list(*ring, state);
    values[&polygon] = state.dim;
  }
  ast::Dim curves(const auto &members, State &state) {
    auto dim = ast::Dim::none;
    for (const auto &member : members) {
      if (const auto *sequence = std::get_if<ast::PointList *>(&member);
          sequence && *sequence) {
        list(**sequence, state);
        dim = combine(dim, get(**sequence));
      } else if (const auto *geometry = std::get_if<ast::Geometry *>(&member);
                 geometry && *geometry) {
        visit(**geometry, state);
        dim = combine(dim, get(**geometry));
      }
    }
    return dim;
  }
  ast::Dim surfaces(const auto &members, State &state) {
    auto dim = ast::Dim::none;
    for (const auto &member : members) {
      if (const auto *bare = std::get_if<ast::BarePolygon *>(&member);
          bare && *bare) {
        polygon(**bare, state);
        dim = combine(dim, get(**bare));
      } else if (const auto *geometry = std::get_if<ast::Geometry *>(&member);
                 geometry && *geometry) {
        visit(**geometry, state);
        dim = combine(dim, get(**geometry));
      }
    }
    return dim;
  }
  void visit(const ast::Geometry &geometry, State outer) {
    State state{geometry.dim, geometry.dim != ast::Dim::none};
    auto dim = state.dim;
    if (const auto *g = cursiv::ast_cast<ast::PointGeom>(&geometry)) {
      if (g->value)
        point(*g->value, state);
      dim = state.dim;
    } else if (const auto *g =
                   cursiv::ast_cast<ast::PointListGeometry>(&geometry)) {
      points(g->points, state);
      dim = state.dim;
    } else if (const auto *g = cursiv::ast_cast<ast::PolygonGeom>(&geometry)) {
      for (const auto *ring : g->rings)
        if (ring)
          list(*ring, state);
      dim = state.dim;
    } else if (const auto *g =
                   cursiv::ast_cast<ast::MultiPointGeom>(&geometry)) {
      dim = ast::Dim::none;
      for (const auto *item : g->items)
        if (item) {
          marker(*item, item->dim, state);
          if (item->value)
            point(*item->value, state);
          values[item] = state.dim;
          dim = combine(dim, state.dim);
        }
    } else if (const auto *g =
                   cursiv::ast_cast<ast::MultiLineStringGeom>(&geometry)) {
      dim = ast::Dim::none;
      for (const auto *line : g->lines)
        if (line) {
          list(*line, state);
          dim = combine(dim, get(*line));
        }
    } else if (const auto *g =
                   cursiv::ast_cast<ast::MultiPolygonGeom>(&geometry)) {
      dim = ast::Dim::none;
      for (const auto *p : g->polygons)
        if (p) {
          polygon(*p, state);
          dim = combine(dim, get(*p));
        }
    } else if (const auto *g =
                   cursiv::ast_cast<ast::CompoundCurveGeom>(&geometry)) {
      dim = curves(g->members, state);
    } else if (const auto *g =
                   cursiv::ast_cast<ast::CurvePolygonGeom>(&geometry)) {
      if (!g->members.empty())
        dim = curves(g->members, state);
    } else if (const auto *g =
                   cursiv::ast_cast<ast::MultiCurveGeom>(&geometry)) {
      dim = curves(g->members, state);
    } else if (const auto *g =
                   cursiv::ast_cast<ast::MultiSurfaceGeom>(&geometry)) {
      dim = surfaces(g->members, state);
    } else if (const auto *g =
                   cursiv::ast_cast<ast::GeometryCollectionGeom>(&geometry)) {
      dim = ast::Dim::none;
      for (const auto *child : g->geometries)
        if (child) {
          visit(*child, state);
          dim = combine(dim, get(*child));
        }
    } else {
      values[&geometry] = geometry.dim;
      return;
    }
    values[&geometry] = dim;
    if (outer.fixed && state.dim != outer.dim)
      errors.push_back({&geometry, Error::child});
  }
  std::unordered_map<const cursiv::AstNode *, ast::Dim> values;
  std::vector<Issue> errors;
};
} // namespace wkt
