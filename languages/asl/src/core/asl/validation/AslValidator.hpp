// The checks of this language: what the grammar accepts but the language
// does not.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "asl/ast.hpp"

namespace asl {

class AslValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct AslCoreServices;
void register_validation_checks(AslCoreServices& services);

}  // namespace asl
