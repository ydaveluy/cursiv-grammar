#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "tinyc/ast.hpp"

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
