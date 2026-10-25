#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "terraform/core/ast.hpp"

namespace terraform {

class TerraformValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkFile(const ast::File&, cursiv::ValidationAcceptor&) const;
  void checkBlock(const ast::Block&, cursiv::ValidationAcceptor&) const;
  void checkFor(const ast::ForLoop&, cursiv::ValidationAcceptor&) const;
  void checkUnary(const ast::UnaryExpr&, cursiv::ValidationAcceptor&) const;
  void checkTemplate(const ast::StringLit&, cursiv::ValidationAcceptor&) const;
  void checkText(const ast::TemplateText&, cursiv::ValidationAcceptor&) const;
};

struct TerraformCoreServices;
void register_validation_checks(TerraformCoreServices& services);

}  // namespace terraform
