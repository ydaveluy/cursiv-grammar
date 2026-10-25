#pragma once
#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "prolog/core/ast.hpp"

namespace prolog {
class PrologValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkTerm(const ast::Term &, cursiv::ValidationAcceptor &) const;
};
struct PrologCoreServices;
void register_validation_checks(PrologCoreServices &services);
} // namespace prolog
