// The checks of this language: none yet -- nothing here needs a check
// beyond parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "smalltalk/core/ast.hpp"

namespace smalltalk {

class SmalltalkValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct SmalltalkCoreServices;
void register_validation_checks(SmalltalkCoreServices& services);

}  // namespace smalltalk
