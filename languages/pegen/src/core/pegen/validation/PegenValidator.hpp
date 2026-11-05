#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "pegen/ast.hpp"

namespace pegen {

class PegenValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkGrammar(const ast::Grammar&, cursiv::ValidationAcceptor&) const;
};

struct PegenCoreServices;
void register_validation_checks(PegenCoreServices& services);

}  // namespace pegen
