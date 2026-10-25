// The checks of this language: the grammar is the whole of it, so there are
// none yet -- the pair exists for whoever adds the first one.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "gml/core/ast.hpp"

namespace gml {

class GmlValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkEntry(const ast::Kv&, cursiv::ValidationAcceptor&) const;
};

struct GmlCoreServices;
void register_validation_checks(GmlCoreServices& services);

}  // namespace gml
