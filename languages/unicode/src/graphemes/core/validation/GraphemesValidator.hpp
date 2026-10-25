// The checks of this language: what the grammar accepts but the language
// does not. None yet: nothing here needs a check beyond parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "graphemes/core/ast.hpp"

namespace graphemes {

class GraphemesValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct GraphemesCoreServices;
void register_validation_checks(GraphemesCoreServices& services);

}  // namespace graphemes
