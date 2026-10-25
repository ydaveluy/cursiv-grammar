// The checks of this language: what the grammar accepts but the language
// does not. None yet: nothing here needs a check beyond parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "logo/core/ast.hpp"

namespace logo {

class LogoValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct LogoCoreServices;
void register_validation_checks(LogoCoreServices& services);

}  // namespace logo
