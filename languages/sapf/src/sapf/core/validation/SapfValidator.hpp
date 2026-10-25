// The checks of this language: what the grammar accepts but the language does
// not.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "sapf/core/ast.hpp"

namespace sapf {

class SapfValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkUniqueParams(const ast::FunctionLiteral& function,
                         cursiv::ValidationAcceptor& accept) const;
};

struct SapfCoreServices;
void register_validation_checks(SapfCoreServices& services);

}  // namespace sapf
