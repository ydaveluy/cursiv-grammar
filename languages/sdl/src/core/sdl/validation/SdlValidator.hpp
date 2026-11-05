// Contextual names and language-list consistency from the native front end.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "sdl/ast.hpp"

namespace sdl {

class SdlValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkModule(const ast::Module&, cursiv::ValidationAcceptor&) const;
  void checkAggregate(const ast::AggregateStmt&, cursiv::ValidationAcceptor&) const;
  void checkNestedAggregate(const ast::NestedAggregate&, cursiv::ValidationAcceptor&) const;
  void checkConditional(const ast::ConditionalStmt&, cursiv::ValidationAcceptor&) const;

};

struct SdlCoreServices;
void register_validation_checks(SdlCoreServices& services);

}  // namespace sdl
