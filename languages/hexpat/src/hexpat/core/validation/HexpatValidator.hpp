// Native ordering and type restrictions for function parameters.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "hexpat/core/ast.hpp"

namespace hexpat {

class HexpatValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkParameters(const ast::Fn&, cursiv::ValidationAcceptor&) const;
  void checkString(const ast::StringLit&, cursiv::ValidationAcceptor&) const;
  void checkCharacter(const ast::CharLit&, cursiv::ValidationAcceptor&) const;
};

struct HexpatCoreServices;
void register_validation_checks(HexpatCoreServices& services);

}  // namespace hexpat
