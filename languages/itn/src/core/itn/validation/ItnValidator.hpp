#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "itn/ast.hpp"

namespace itn {

class ItnValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkLine(const ast::Line&, cursiv::ValidationAcceptor&) const;
};

struct ItnCoreServices;
void register_validation_checks(ItnCoreServices& services);

}  // namespace itn
