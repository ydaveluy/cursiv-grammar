// The checks of this language: what the grammar accepts but a well-formed
// document should not repeat.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "mckeemanform/core/ast.hpp"

namespace mckeemanform {

class MckeemanFormValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkUniqueRuleNames(const ast::Document& document,
                            cursiv::ValidationAcceptor& accept) const;
  void checkRange(const ast::Range& range,
                  cursiv::ValidationAcceptor& accept) const;
  void checkAlternative(const ast::Alternative& alternative,
                        cursiv::ValidationAcceptor& accept) const;
};

struct MckeemanFormCoreServices;
void register_validation_checks(MckeemanFormCoreServices& services);

}  // namespace mckeemanform
