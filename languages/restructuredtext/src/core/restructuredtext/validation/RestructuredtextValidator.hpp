// The checks of this language: what the grammar accepts but the language does
// not.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "restructuredtext/ast.hpp"

namespace restructuredtext {

class RestructuredtextValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkLiteralNotEmpty(const ast::Literal& literal,
                            cursiv::ValidationAcceptor& accept) const;

};

struct RestructuredtextCoreServices;
void register_validation_checks(RestructuredtextCoreServices& services);

}  // namespace restructuredtext
