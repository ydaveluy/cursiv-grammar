// The checks of this language: what the grammar accepts but the language
// does not. One here, a fact no single node can see on its own.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "nasm/core/ast.hpp"

namespace nasm {

class NasmValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkUniqueLabels(const ast::Program& program,
                         cursiv::ValidationAcceptor& accept) const;
};

struct NasmCoreServices;
void register_validation_checks(NasmCoreServices& services);

}  // namespace nasm
