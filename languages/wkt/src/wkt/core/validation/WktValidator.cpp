#include "wkt/core/validation/WktValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "wkt/core/CoreServices.hpp"
#include "wkt/core/Numbers.hpp"
#include "wkt/core/Dimensions.hpp"

namespace wkt {
namespace {
bool same_xy(const ast::Point* a, const ast::Point* b) {
  if (!a || !b || a->ordinates.size() < 2 || b->ordinates.size() < 2) return false;
  for (std::size_t i = 0; i < 2; ++i) {
    const auto x = ordinate_value(a->ordinates[i]), y = ordinate_value(b->ordinates[i]);
    if (!x || !y || *x != *y) return false;
  }
  return true;
}
void check_ring(const cursiv::AstNode& node, const auto& points,
                cursiv::ValidationAcceptor& accept) {
  if (points.empty()) return;
  if (points.size() < 3)
    accept.error(node, "A nonempty ring needs at least three points.")
        .code("wkt.ring-size");
  if (!same_xy(points[0], points.back()))
    accept.error(node, "The ring must end at its starting coordinate.")
        .code("wkt.ring-closure");
}
}  // namespace

void WktValidator::checkDocument(const ast::Document &document,
                                 cursiv::ValidationAcceptor &accept) const {
  const Dimensions dimensions(document);
  for (const auto &issue : dimensions.issues()) {
    if (issue.error == Dimensions::Error::marker) {
      const auto report = [&]<class T>(const T &node) {
        accept
            .error(node,
                   "A dimension marker was already specified or inferred.")
            .code("wkt.dimension-marker")
            .template property<&T::dim>();
      };
      if (const auto *node = cursiv::ast_cast<ast::PointList>(issue.node))
        report(*node);
      else if (const auto *node =
                   cursiv::ast_cast<ast::BarePolygon>(issue.node))
        report(*node);
      else if (const auto *node =
                   cursiv::ast_cast<ast::MultiPointItem>(issue.node))
        report(*node);
    } else if (issue.error == Dimensions::Error::coordinate)
      accept
          .error(*issue.node, "Coordinate dimension differs from the geometry.")
          .code("wkt.dimension");
    else if (const auto *node = cursiv::ast_cast<ast::Geometry>(issue.node))
      accept.error(*node, "The child geometry has a different dimension.")
          .code("wkt.child-dimension")
          .property<&ast::Geometry::dim>();
  }
}

void WktValidator::checkPoint(const ast::Point& point,
                              cursiv::ValidationAcceptor& accept) const {
  if (point.ordinates.size() < 2 || point.ordinates.size() > 4)
    accept.error(point, "A coordinate needs two to four ordinates.")
        .code("wkt.coordinate-arity");
  for (std::size_t i = 0; i < point.ordinates.size(); ++i)
    if (!ordinate_value(point.ordinates[i]))
      accept.error(point, "Invalid numeric ordinate.")
          .code("wkt.ordinate")
          .property<&ast::Point::ordinates>(i);
}

void WktValidator::checkGeometry(const ast::Geometry& geometry,
                                 cursiv::ValidationAcceptor& accept) const {
  if (const auto* compound = cursiv::ast_cast<ast::CompoundCurveGeom>(&geometry)) {
    const ast::Point* previous = nullptr;
    for (std::size_t i = 0; i < compound->members.size(); ++i) {
      const auto& member = compound->members[i];
      const cursiv::vector<ast::Point*>* sequence = nullptr;
      if (const auto* list = std::get_if<ast::PointList*>(&member); list && *list)
        sequence = &(*list)->points;
      else if (const auto* child = std::get_if<ast::Geometry*>(&member);
               child && *child) {
        if (const auto* line = cursiv::ast_cast<ast::LineStringGeom>(*child))
          sequence = &line->points;
        else if (const auto* ring = cursiv::ast_cast<ast::LinearRingGeom>(*child))
          sequence = &ring->points;
        else if (const auto* circle = cursiv::ast_cast<ast::CircularStringGeom>(*child))
          sequence = &circle->points;
      }
      if (!sequence) {
        previous = nullptr;
        continue;
      }
      if (sequence->empty()) {
        accept.error(*compound, "A compound curve cannot contain empty sections.")
            .code("wkt.empty-curve-section")
            .property<&ast::CompoundCurveGeom::members>(i);
        previous = nullptr;
      } else {
        if (previous && !same_xy(previous, (*sequence)[0]))
          accept.error(*compound, "The curve sections must meet at their endpoints.")
              .code("wkt.curve-continuity")
              .property<&ast::CompoundCurveGeom::members>(i);
        previous = sequence->back();
      }
    }
  }
  if (const auto* polygon = cursiv::ast_cast<ast::PolygonGeom>(&geometry)) {
    if (polygon->rings.size() > 1 && polygon->rings[0] &&
        polygon->rings[0]->points.empty()) {
      for (std::size_t i = 1; i < polygon->rings.size(); ++i)
        if (polygon->rings[i] && !polygon->rings[i]->points.empty()) {
          accept.error(geometry, "An empty shell cannot contain nonempty holes.")
              .code("wkt.empty-shell");
          break;
        }
    }
  }
  if (const auto* line = cursiv::ast_cast<ast::LineStringGeom>(&geometry)) {
    if (line->points.size() == 1)
      accept.error(geometry, "A nonempty line string needs at least two points.")
          .code("wkt.line-size");
  } else if (const auto* ring = cursiv::ast_cast<ast::LinearRingGeom>(&geometry)) {
    check_ring(geometry, ring->points, accept);
  } else if (const auto* curve = cursiv::ast_cast<ast::CircularStringGeom>(&geometry)) {
    if (curve->points.size() == 2)
      accept.error(geometry, "A circular string cannot have exactly two points.")
          .code("wkt.circular-size");
  }
}

void WktValidator::checkPointList(const ast::PointList& list,
                                  cursiv::ValidationAcceptor& accept) const {
  if (cursiv::ast_cast<ast::PolygonGeom>(list.parent) ||
      cursiv::ast_cast<ast::BarePolygon>(list.parent))
    check_ring(list, list.points, accept);
  else if (list.points.size() == 1)
    accept.error(list, "A nonempty line string needs at least two points.")
        .code("wkt.line-size");
}

void register_validation_checks(WktCoreServices& services) {
  using V = WktValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkDocument>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkPoint>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkGeometry>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkPointList>(validator)});
}
}  // namespace wkt
