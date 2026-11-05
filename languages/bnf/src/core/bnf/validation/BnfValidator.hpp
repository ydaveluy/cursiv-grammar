// The checks of this language. None yet: a repeated production name and an
// empty right-hand side are both ordinary BNF style here, not a mistake.
#pragma once

#include "bnf/ast.hpp"
#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"

namespace bnf {

class BnfValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct BnfCoreServices;
void register_validation_checks(BnfCoreServices& services);

}  // namespace bnf
