#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "suokif/core/ast.hpp"

namespace suokif {
class SuokifValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkDocument(const ast::Document&, cursiv::ValidationAcceptor&) const;
  void checkList(const ast::List&, cursiv::ValidationAcceptor&) const;
};
struct SuokifCoreServices;
void register_validation_checks(SuokifCoreServices& services);
}  // namespace suokif
