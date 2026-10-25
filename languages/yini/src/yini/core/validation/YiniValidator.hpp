#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "yini/core/ast.hpp"

namespace yini {
class YiniValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkDocument(const ast::Document&, cursiv::ValidationAcceptor&) const;
  void checkString(const ast::String&, cursiv::ValidationAcceptor&) const;
  void checkNumber(const ast::Number&, cursiv::ValidationAcceptor&) const;
  void checkList(const ast::List&, cursiv::ValidationAcceptor&) const;
  void checkConcat(const ast::Concat&, cursiv::ValidationAcceptor&) const;
  void checkObject(const ast::Object&, cursiv::ValidationAcceptor&) const;
};
struct YiniCoreServices;
void register_validation_checks(YiniCoreServices& services);
}  // namespace yini
