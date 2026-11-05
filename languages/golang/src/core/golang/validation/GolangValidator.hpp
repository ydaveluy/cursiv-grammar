// The checks of this language: what the grammar accepts but the language
// does not. None yet -- this parser has no symbol table to check against.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "golang/ast.hpp"

namespace golang {

class GolangValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct GolangCoreServices;
void register_validation_checks(GolangCoreServices& services);

}  // namespace golang
