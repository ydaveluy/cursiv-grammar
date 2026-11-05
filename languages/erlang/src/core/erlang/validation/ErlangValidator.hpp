// The checks of this language: none yet -- nothing here needs a check
// beyond parsing.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "erlang/ast.hpp"

namespace erlang {

class ErlangValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct ErlangCoreServices;
void register_validation_checks(ErlangCoreServices& services);

}  // namespace erlang
