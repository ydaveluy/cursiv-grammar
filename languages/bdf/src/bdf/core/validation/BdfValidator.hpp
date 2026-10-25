#pragma once
#include "bdf/core/ast.hpp"
#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
namespace bdf {
class BdfValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkPropertiesCount(const ast::Properties&, cursiv::ValidationAcceptor&) const;
  void checkCharsCount(const ast::Font&, cursiv::ValidationAcceptor&) const;
  void checkBitmap(const ast::Char&, cursiv::ValidationAcceptor&) const;
  void checkDirective(const ast::Directive&, cursiv::ValidationAcceptor&) const;
};
struct BdfCoreServices;
void register_validation_checks(BdfCoreServices&);
}  // namespace bdf
