// The checks of this language. None yet: the grammar alone already rejects
// what BNF forbids.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "ebnf/core/ast.hpp"

namespace ebnf {

class EbnfValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct EbnfCoreServices;
void register_validation_checks(EbnfCoreServices& services);

}  // namespace ebnf
