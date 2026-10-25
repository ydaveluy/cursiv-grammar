// The checks of this language: none yet -- nothing here needs a check
// beyond parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "erlang/core/ast.hpp"

namespace erlang {

class ErlangValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct ErlangCoreServices;
void register_validation_checks(ErlangCoreServices& services);

}  // namespace erlang
