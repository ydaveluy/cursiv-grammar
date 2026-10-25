#pragma once

#include "capnproto/core/ast.hpp"
#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"

namespace capnproto {

class CapnprotoValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkDocument(const ast::Document&, cursiv::ValidationAcceptor&) const;
  void checkDeclaration(const ast::Declaration&,
                        cursiv::ValidationAcceptor&) const;
  void checkId(const ast::Id&, cursiv::ValidationAcceptor&) const;
  void checkInteger(const ast::IntegerLiteral&,
                    cursiv::ValidationAcceptor&) const;
  void checkBytes(const ast::BytesLiteral&, cursiv::ValidationAcceptor&) const;
  void checkNegative(const ast::NegativeExpr&,
                     cursiv::ValidationAcceptor&) const;
  void checkTargets(const ast::AnnotationDef&,
                    cursiv::ValidationAcceptor&) const;
};

struct CapnprotoCoreServices;
void register_validation_checks(CapnprotoCoreServices& services);

}  // namespace capnproto
