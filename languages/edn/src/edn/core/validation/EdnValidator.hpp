// Reader constraints that are not expressed by the structural grammar.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "edn/core/ast.hpp"

namespace edn {

class EdnValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkCharacter(const ast::Character&, cursiv::ValidationAcceptor&) const;
  void checkMap(const ast::Map&, cursiv::ValidationAcceptor&) const;
  void checkSet(const ast::Set&, cursiv::ValidationAcceptor&) const;
};

struct EdnCoreServices;
void register_validation_checks(EdnCoreServices& services);

}  // namespace edn
