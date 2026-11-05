// Recursive type arities and vector dimensions are checked after parsing.
#pragma once

#include "cql3/ast.hpp"
#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"

namespace cql3 {

class Cql3Validator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkColumnType(const ast::ColumnType&,
                       cursiv::ValidationAcceptor&) const;
};

struct Cql3CoreServices;
void register_validation_checks(Cql3CoreServices& services);

}  // namespace cql3
