// The checks of this language.
#pragma once

#include "atl/core/ast.hpp"
#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"

namespace atl {

class AtlValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkModifiers(const ast::MatchedRule&, cursiv::ValidationAcceptor&) const;

  void checkRuleNotSelfExtending(const ast::MatchedRule& rule,
                                 cursiv::ValidationAcceptor& accept) const;
};

struct AtlCoreServices;
void register_validation_checks(AtlCoreServices& services);

}  // namespace atl
