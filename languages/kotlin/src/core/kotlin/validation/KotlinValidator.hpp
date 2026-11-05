// The checks of this language: what the grammar accepts but the language
// does not. Empty for now -- registered so a future check has a home.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "kotlin/ast.hpp"

namespace kotlin {

class KotlinValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct KotlinCoreServices;
void register_validation_checks(KotlinCoreServices& services);

}  // namespace kotlin
