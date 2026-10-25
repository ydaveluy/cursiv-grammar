// The checks of this language: what the grammar accepts but the language
// does not.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "javacc/core/ast.hpp"

namespace javacc {

class JavaccValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkParserNamesMatch(const ast::JavaccInput& input,
                             cursiv::ValidationAcceptor& accept) const;
};

struct JavaccCoreServices;
void register_validation_checks(JavaccCoreServices& services);

}  // namespace javacc
