#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "sparql/core/ast.hpp"

namespace sparql {

class SparqlValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkBuiltInCall(const ast::BuiltInCall &,
                        cursiv::ValidationAcceptor &) const;
  void checkPrefixesDeclared(const ast::Query &query,
                             cursiv::ValidationAcceptor &accept) const;
};

struct SparqlCoreServices;
void register_validation_checks(SparqlCoreServices &services);

} // namespace sparql
