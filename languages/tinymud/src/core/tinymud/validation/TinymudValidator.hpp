// Syntax-level flag/password checks; no game state or privilege resolution.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "tinymud/ast.hpp"

namespace tinymud {

class TinymudValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkSet(const ast::SetCommand&, cursiv::ValidationAcceptor&) const;
  void checkPlayer(const ast::PcreateCommand&, cursiv::ValidationAcceptor&) const;
};

struct TinymudCoreServices;
void register_validation_checks(TinymudCoreServices& services);

}  // namespace tinymud
