#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "suokif/ast.hpp"

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
