// The checks of this language: none -- a call's name and arguments carry no
// symbol table or convention a grammar-level check could judge.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "callable/core/ast.hpp"

namespace callable {

class CallableValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct CallableCoreServices;
void register_validation_checks(CallableCoreServices& services);

}  // namespace callable
