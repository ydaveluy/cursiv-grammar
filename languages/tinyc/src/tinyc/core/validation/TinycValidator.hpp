#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "tinyc/core/ast.hpp"

namespace tinyc {

class TinycValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkProgram(const ast::Program &, cursiv::ValidationAcceptor &) const;
  void checkIdentifier(const ast::Ident &, cursiv::ValidationAcceptor &) const;
};

struct TinycCoreServices;
void register_validation_checks(TinycCoreServices &services);

} // namespace tinyc
