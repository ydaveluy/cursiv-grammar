// The checks of this language: what the grammar accepts but a valid map does
// not -- a plane needs exactly 3 points, a point exactly 3 numbers.
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
