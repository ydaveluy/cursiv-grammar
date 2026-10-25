// The checks of this language: none -- an s-expression's shape is exactly
// what the grammar accepts, and there is no symbol table to judge.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"

namespace lisp {

class LispValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct LispCoreServices;
void register_validation_checks(LispCoreServices& services);

}  // namespace lisp
