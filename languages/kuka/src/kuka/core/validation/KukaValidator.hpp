// The checks of this language: what the grammar accepts but the language
// does not. One here -- two parameters of a routine sharing a name.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "kuka/core/ast.hpp"

namespace kuka {

class KukaValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkUniqueParams(const ast::Routine& routine,
                         cursiv::ValidationAcceptor& accept) const;
};

struct KukaCoreServices;
void register_validation_checks(KukaCoreServices& services);

}  // namespace kuka
