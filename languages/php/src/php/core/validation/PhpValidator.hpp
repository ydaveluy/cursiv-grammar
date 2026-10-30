// Context-sensitive native naming checks; member keywords remain legal.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "php/core/ast.hpp"

namespace php {

class PhpValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkFunction(const ast::FunctionDecl&, cursiv::ValidationAcceptor&) const;
  void checkClass(const ast::ClassDecl&, cursiv::ValidationAcceptor&) const;
  void checkEnum(const ast::EnumDecl&, cursiv::ValidationAcceptor&) const;
};

struct PhpCoreServices;
void register_validation_checks(PhpCoreServices& services);

}  // namespace php
