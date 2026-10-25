#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "itn/core/ast.hpp"

namespace itn {

class ItnValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkLine(const ast::Line&, cursiv::ValidationAcceptor&) const;
};

struct ItnCoreServices;
void register_validation_checks(ItnCoreServices& services);

}  // namespace itn
