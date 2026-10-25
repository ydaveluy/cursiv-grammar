// The checks of this language: what the grammar accepts but the language
// does not. Empty for now -- registered so a future check has a home.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "kotlin/core/ast.hpp"

namespace kotlin {

class KotlinValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct KotlinCoreServices;
void register_validation_checks(KotlinCoreServices& services);

}  // namespace kotlin
