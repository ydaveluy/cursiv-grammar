// The checks of this language: what the grammar accepts but a real Canadian
// postal code does not.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "postalcode/core/ast.hpp"

namespace postalcode {

class PostalcodeValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkNoExcludedLetters(const ast::PostalCode& code,
                              cursiv::ValidationAcceptor& accept) const;
};

struct PostalcodeCoreServices;
void register_validation_checks(PostalcodeCoreServices& services);

}  // namespace postalcode
