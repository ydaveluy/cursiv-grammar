// The checks of this language: nothing yet -- this port covers syntax only.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "flatbuffers/core/ast.hpp"

namespace flatbuffers {

class FlatbuffersValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct FlatbuffersCoreServices;
void register_validation_checks(FlatbuffersCoreServices& services);

}  // namespace flatbuffers
