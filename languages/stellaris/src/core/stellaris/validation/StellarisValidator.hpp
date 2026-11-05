#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "stellaris/ast.hpp"

namespace stellaris {
class StellarisValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkGroup(const ast::Group& node, cursiv::ValidationAcceptor& accept) const;
  void checkOperator(const ast::KeyValue& node, cursiv::ValidationAcceptor& accept) const;
  void checkDocument(const ast::Document& node, cursiv::ValidationAcceptor& accept) const;
};
struct StellarisCoreServices;
void register_validation_checks(StellarisCoreServices& services);
}  // namespace stellaris
