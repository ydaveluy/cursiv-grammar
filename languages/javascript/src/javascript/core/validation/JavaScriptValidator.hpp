// The checks of this language: what the grammar accepts but the language
// does not. Strict mode is one: real JS gates eight words and the legacy
// octal on whether the code is strict; here the grammar reads them
// everywhere and the code's own strictness judges after.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "javascript/core/ast.hpp"

namespace javascript {

class JavaScriptValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  // `let`, `static`, `implements`... as a binding or a reference in strict
  // code. A property name is any word and is left alone.
  void checkStrictName(const ast::Ident& ident,
                       cursiv::ValidationAcceptor& accept) const;
  // `0755` in strict code.
  void checkLegacyOctal(const ast::NumberLit& lit,
                        cursiv::ValidationAcceptor& accept) const;
};

struct JavaScriptCoreServices;
void register_validation_checks(JavaScriptCoreServices& services);

}  // namespace javascript
