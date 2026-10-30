// Defensive shape checks for brush-plane and point nodes.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "quakemap/core/ast.hpp"

namespace quakemap {

class QuakemapValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkPlaneCoordCount(const ast::BrushLine& line,
                            cursiv::ValidationAcceptor& accept) const;
  void checkCoordComponents(const ast::Coord& coord,
                            cursiv::ValidationAcceptor& accept) const;
};

struct QuakemapCoreServices;
void register_validation_checks(QuakemapCoreServices& services);

}  // namespace quakemap
