// The checks of this language: what the grammar accepts but the language does
// not.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "restructuredtext/core/ast.hpp"

namespace restructuredtext {

class RestructuredtextValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkLiteralNotEmpty(const ast::Literal& literal,
                            cursiv::ValidationAcceptor& accept) const;
  void checkHyperlinkHasUrl(const ast::Hyperlink& hyperlink,
                            cursiv::ValidationAcceptor& accept) const;
};

struct RestructuredtextCoreServices;
void register_validation_checks(RestructuredtextCoreServices& services);

}  // namespace restructuredtext
