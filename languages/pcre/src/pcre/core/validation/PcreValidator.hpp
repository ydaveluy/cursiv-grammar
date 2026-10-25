#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "pcre/core/ast.hpp"

namespace pcre {
class PcreValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkQuantified(const ast::Quantified &,
                       cursiv::ValidationAcceptor &) const;
  void checkLiteral(const ast::Literal &, cursiv::ValidationAcceptor &) const;
  void checkClass(const ast::CharacterClass &,
                  cursiv::ValidationAcceptor &) const;
  void checkClassType(const ast::CharacterType &,
                      cursiv::ValidationAcceptor &) const;
  void checkRange(const ast::CharacterClassRange &,
                  cursiv::ValidationAcceptor &) const;
  void checkPosix(const ast::PosixCharacterClass &,
                  cursiv::ValidationAcceptor &) const;
  void checkGroup(const ast::Group &, cursiv::ValidationAcceptor &) const;
  void checkCallout(const ast::Callout &, cursiv::ValidationAcceptor &) const;
  void checkVerb(const ast::BacktrackingControl &,
                 cursiv::ValidationAcceptor &) const;
};
struct PcreCoreServices;
void register_validation_checks(PcreCoreServices &services);
} // namespace pcre
