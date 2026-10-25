// The checks of this language: what the grammar accepts but the language
// does not. None yet: nothing here needs a check beyond parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "icon/core/ast.hpp"

namespace icon {

class IconValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct IconCoreServices;
void register_validation_checks(IconCoreServices& services);

}  // namespace icon
