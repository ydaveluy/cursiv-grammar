// The checks of this language: what the grammar accepts but the language
// does not. One here -- a loop's opening and closing label must match.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "lolcode/core/ast.hpp"

namespace lolcode {

class LolcodeValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkLoopLabelsMatch(const ast::LoopStmt& loop,
                            cursiv::ValidationAcceptor& accept) const;
};

struct LolcodeCoreServices;
void register_validation_checks(LolcodeCoreServices& services);

}  // namespace lolcode
