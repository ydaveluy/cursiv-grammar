// The checks of this language: what the grammar accepts but the language
// does not. One here, a fact no single node can see on its own.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "nasm/ast.hpp"

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
