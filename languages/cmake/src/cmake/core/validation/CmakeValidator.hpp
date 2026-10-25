#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "cmake/core/ast.hpp"

namespace cmake {

class CmakeValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkArgument(const ast::Argument&, cursiv::ValidationAcceptor&) const;
  void checkFile(const ast::File&, cursiv::ValidationAcceptor&) const;
};

struct CmakeCoreServices;
void register_validation_checks(CmakeCoreServices& services);

}  // namespace cmake
