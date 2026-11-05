#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "newick/ast.hpp"

namespace newick {

class NewickValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkClade(const ast::Clade&, cursiv::ValidationAcceptor&) const;
};

struct NewickCoreServices;
void register_validation_checks(NewickCoreServices& services);

}  // namespace newick
