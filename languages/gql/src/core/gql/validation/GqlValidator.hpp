// No semantic checks are registered yet. Syntax acceptance does not establish
// GQL type, name/scope, graph/schema or optional-feature conformance.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "gql/ast.hpp"

namespace gql {

class GqlValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct GqlCoreServices;
void register_validation_checks(GqlCoreServices& services);

}  // namespace gql
