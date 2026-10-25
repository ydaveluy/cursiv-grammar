// The checks of this language: none yet. Line numbers may repeat and GOTO
// targets are read at runtime, not resolved here, so nothing in this AST
// needs a check beyond parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "basic/core/ast.hpp"

namespace basic {

class BasicValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct BasicCoreServices;
void register_validation_checks(BasicCoreServices& services);

}  // namespace basic
