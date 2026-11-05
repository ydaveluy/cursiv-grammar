#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "wkt/ast.hpp"

namespace wkt {
class WktValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkDocument(const ast::Document&, cursiv::ValidationAcceptor&) const;
  void checkPoint(const ast::Point&, cursiv::ValidationAcceptor&) const;
  void checkGeometry(const ast::Geometry&, cursiv::ValidationAcceptor&) const;
  void checkPointList(const ast::PointList&, cursiv::ValidationAcceptor&) const;
};
struct WktCoreServices;
void register_validation_checks(WktCoreServices& services);
}  // namespace wkt
