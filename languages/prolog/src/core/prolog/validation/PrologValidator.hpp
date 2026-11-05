#pragma once
#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "prolog/ast.hpp"

namespace prolog {
class PrologValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkTerm(const ast::Term &, cursiv::ValidationAcceptor &) const;
};
struct PrologCoreServices;
void register_validation_checks(PrologCoreServices &services);
} // namespace prolog
