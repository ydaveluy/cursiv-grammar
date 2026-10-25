#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "pegen/core/ast.hpp"

namespace pegen {

class PegenValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkGrammar(const ast::Grammar&, cursiv::ValidationAcceptor&) const;
};

struct PegenCoreServices;
void register_validation_checks(PegenCoreServices& services);

}  // namespace pegen
