// The checks of this language: a fact no single node can see -- a row's
// field count against the header's.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "tsv/ast.hpp"

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
