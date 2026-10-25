#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "rfc1960/core/ast.hpp"

namespace rfc1960 {

class Rfc1960Validator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkSimple(const ast::Simple &, cursiv::ValidationAcceptor &) const;
  void checkSubstring(const ast::Substring &,
                      cursiv::ValidationAcceptor &) const;
  void checkExtensible(const ast::Extensible &,
                       cursiv::ValidationAcceptor &) const;
  void checkNot(const ast::Not &, cursiv::ValidationAcceptor &) const;
};

struct Rfc1960CoreServices;
void register_validation_checks(Rfc1960CoreServices &services);

} // namespace rfc1960
