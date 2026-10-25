#pragma once
#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "guido/core/ast.hpp"
namespace guido {
class GuidoValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkNote(const ast::Note &, cursiv::ValidationAcceptor &) const;
  void checkDuration(const ast::Duration &, cursiv::ValidationAcceptor &) const;
  void checkParameter(const ast::Parameter &,
                      cursiv::ValidationAcceptor &) const;
  void checkChordMember(const ast::ChordMember &,
                        cursiv::ValidationAcceptor &) const;
};
struct GuidoCoreServices;
void register_validation_checks(GuidoCoreServices &);
} // namespace guido
