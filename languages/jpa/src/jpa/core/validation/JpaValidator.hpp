// The checks of this language. Empty for now: the grammar surface (path
// expressions collapsing every field flavor into one shape) leaves nothing
// a single node, or the whole tree, can flag without a symbol table.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "jpa/core/ast.hpp"

namespace jpa {

class JpaValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct JpaCoreServices;
void register_validation_checks(JpaCoreServices& services);

}  // namespace jpa
