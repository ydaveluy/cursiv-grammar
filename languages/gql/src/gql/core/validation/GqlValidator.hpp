// No semantic checks are registered yet. Syntax acceptance does not establish
// GQL type, name/scope, graph/schema or optional-feature conformance.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "gql/core/ast.hpp"

namespace gql {

class GqlValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct GqlCoreServices;
void register_validation_checks(GqlCoreServices& services);

}  // namespace gql
