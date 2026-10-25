// The checks of this language: a fact no single node can see -- a row's
// field count against the header's.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "tsv/core/ast.hpp"

namespace tsv {

class TsvValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkRowFieldCount(const ast::Document& doc,
                          cursiv::ValidationAcceptor& accept) const;
};

struct TsvCoreServices;
void register_validation_checks(TsvCoreServices& services);

}  // namespace tsv
