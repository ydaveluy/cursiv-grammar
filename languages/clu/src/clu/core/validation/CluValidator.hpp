// The checks of this language: what the grammar accepts but the language
// does not. One here -- a routine or cluster's closing name must echo its
// opening one, as CLU requires.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "clu/core/ast.hpp"

namespace clu {

class CluValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkRoutineEndName(const ast::Routine& routine,
                           cursiv::ValidationAcceptor& accept) const;
  void checkClusterEndName(const ast::Cluster& cluster,
                           cursiv::ValidationAcceptor& accept) const;
};

struct CluCoreServices;
void register_validation_checks(CluCoreServices& services);

}  // namespace clu
