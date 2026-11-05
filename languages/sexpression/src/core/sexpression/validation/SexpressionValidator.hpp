#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "sexpression/ast.hpp"

namespace sexpression {

class SexpressionValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkList(const ast::List&, cursiv::ValidationAcceptor&) const;
  void checkCharacter(const ast::Character&, cursiv::ValidationAcceptor&) const;
};

struct SexpressionCoreServices;
void register_validation_checks(SexpressionCoreServices& services);

}  // namespace sexpression
