// Native string checks that keep invalid source available in the AST.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "d2/core/ast.hpp"

namespace d2 {

class D2Validator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkChunk(const ast::Chunk& node,
                  cursiv::ValidationAcceptor& accept) const;
};

struct D2CoreServices;
void register_validation_checks(D2CoreServices& services);

}  // namespace d2
