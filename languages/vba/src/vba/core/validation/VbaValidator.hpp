// Local source-line constraints for preprocessing directives.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "vba/core/ast.hpp"

namespace vba {

class VbaValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkCompilationConst(const ast::CompilationConst&,
                             cursiv::ValidationAcceptor&) const;
};

struct VbaCoreServices;
void register_validation_checks(VbaCoreServices& services);

}  // namespace vba
