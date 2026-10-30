#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "racketbsl/core/ast.hpp"

namespace racketbsl {
class RacketBslValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkString(const ast::String&, cursiv::ValidationAcceptor&) const;
  void checkBytes(const ast::Bytes&, cursiv::ValidationAcceptor&) const;
  void checkRegex(const ast::Regex&, cursiv::ValidationAcceptor&) const;
  void checkAtom(const ast::Atom&, cursiv::ValidationAcceptor&) const;
  void checkList(const ast::List &, cursiv::ValidationAcceptor &) const;
  void checkPrefix(const ast::Prefix &, cursiv::ValidationAcceptor &) const;
  void checkCharacter(const ast::Character &,
                      cursiv::ValidationAcceptor &) const;
  void checkGraphRef(const ast::GraphRef &, cursiv::ValidationAcceptor &) const;
  void checkProgram(const ast::Program &, cursiv::ValidationAcceptor &) const;
};
struct RacketBslCoreServices;
void register_validation_checks(RacketBslCoreServices &services);
} // namespace racketbsl
