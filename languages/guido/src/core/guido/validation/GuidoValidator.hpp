#pragma once
#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "guido/ast.hpp"
namespace guido {
class GuidoValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkNote(const ast::Note &, cursiv::ValidationAcceptor &) const;
  void checkTablature(const ast::Tablature &, cursiv::ValidationAcceptor &) const;
  void checkDuration(const ast::Duration &, cursiv::ValidationAcceptor &) const;
  void checkParameter(const ast::Parameter &,
                      cursiv::ValidationAcceptor &) const;
  void checkChordMember(const ast::ChordMember &,
                        cursiv::ValidationAcceptor &) const;
};
struct GuidoCoreServices;
void register_validation_checks(GuidoCoreServices &);
} // namespace guido
