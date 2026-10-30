// Semantic checks keep the unbounded numeric lexemes in the AST.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "alpaca/core/ast.hpp"

namespace alpaca {

class AlpacaValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkAdjacency(const ast::AdjacencyPred& node,
                      cursiv::ValidationAcceptor& accept) const;
};

struct AlpacaCoreServices;
void register_validation_checks(AlpacaCoreServices& services);

}  // namespace alpaca
