#pragma once
#include "aterm/core/ast.hpp"
#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
namespace aterm {
class AtermValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkNumber(const ast::Number &, cursiv::ValidationAcceptor &) const;
};
struct AtermCoreServices;
void register_validation_checks(AtermCoreServices &services);
} // namespace aterm
