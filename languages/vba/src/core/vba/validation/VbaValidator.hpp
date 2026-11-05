// Local source-line constraints for preprocessing directives.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "vba/ast.hpp"

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
