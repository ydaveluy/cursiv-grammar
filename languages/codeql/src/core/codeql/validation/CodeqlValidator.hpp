// The checks of this language: what the grammar accepts but the language does
// not. None yet: nothing here needs a check beyond parsing.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "codeql/ast.hpp"

namespace codeql {

class CodeqlValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct CodeqlCoreServices;
void register_validation_checks(CodeqlCoreServices& services);

}  // namespace codeql
