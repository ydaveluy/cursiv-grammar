#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "sieve/core/ast.hpp"

namespace sieve {
class SieveValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkNumber(const ast::Number &, cursiv::ValidationAcceptor &) const;
  void checkString(const ast::String &, cursiv::ValidationAcceptor &) const;
  void checkStringList(const ast::StringList &,
                       cursiv::ValidationAcceptor &) const;
  void checkTest(const ast::Test &, cursiv::ValidationAcceptor &) const;
  void checkCommand(const ast::Command &, cursiv::ValidationAcceptor &) const;
  void checkScript(const ast::Script &, cursiv::ValidationAcceptor &) const;
};
struct SieveCoreServices;
void register_validation_checks(SieveCoreServices &services);
} // namespace sieve
