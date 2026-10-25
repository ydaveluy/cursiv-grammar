// The checks of this language: what the grammar accepts but the language
// does not. None yet -- this parser has no symbol table to check against.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "golang/core/ast.hpp"

namespace golang {

class GolangValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct GolangCoreServices;
void register_validation_checks(GolangCoreServices& services);

}  // namespace golang
