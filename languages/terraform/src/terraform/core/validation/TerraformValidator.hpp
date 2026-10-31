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
  void checkBinary(const ast::BinaryExpr&, cursiv::ValidationAcceptor&) const;
  void checkConditional(const ast::TernaryExpr&, cursiv::ValidationAcceptor&) const;
  void checkMember(const ast::MemberExpr&, cursiv::ValidationAcceptor&) const;
  void checkIndex(const ast::IndexExpr&, cursiv::ValidationAcceptor&) const;
  void checkSplat(const ast::SplatExpr&, cursiv::ValidationAcceptor&) const;
  void checkCall(const ast::FuncCall&, cursiv::ValidationAcceptor&) const;
  void checkArgument(const ast::Argument&, cursiv::ValidationAcceptor&) const;
  void checkEntry(const ast::MapEntry&, cursiv::ValidationAcceptor&) const;
  void checkTemplate(const ast::StringLit&, cursiv::ValidationAcceptor&) const;
  void checkText(const ast::TemplateText&, cursiv::ValidationAcceptor&) const;
  void checkMap(const ast::MapObj&, cursiv::ValidationAcceptor&) const;
};

struct TerraformCoreServices;
void register_validation_checks(TerraformCoreServices& services);

}  // namespace terraform
