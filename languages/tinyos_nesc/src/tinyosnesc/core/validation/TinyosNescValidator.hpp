// The checks of this language. None yet: the grammar carries no semantic
// rule beyond parsing (no symbol table, no type resolution).
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "tinyosnesc/core/ast.hpp"

namespace tinyosnesc {

class TinyosNescValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct TinyosNescCoreServices;
void register_validation_checks(TinyosNescCoreServices& services);

}  // namespace tinyosnesc
