#pragma once
#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "vhdl/core/ast.hpp"

namespace vhdl {
class VhdlValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkItem(const ast::Item&, cursiv::ValidationAcceptor&) const;
  void checkConcurrent(const ast::ConcStmt&, cursiv::ValidationAcceptor&) const;
  void checkSequential(const ast::SeqStmt&, cursiv::ValidationAcceptor&) const;
  void checkOperator(const ast::BinaryExpr&, cursiv::ValidationAcceptor&) const;
  void checkLiteral(const ast::Literal&, cursiv::ValidationAcceptor&) const;
  void checkIdentifier(const ast::Ident&, cursiv::ValidationAcceptor&) const;
  void checkSubprogram(const ast::SubprogramSpec&, cursiv::ValidationAcceptor&) const;
  void checkFile(const ast::DesignFile&, cursiv::ValidationAcceptor&) const;
};
struct VhdlCoreServices;
void register_validation_checks(VhdlCoreServices& services);
}  // namespace vhdl
