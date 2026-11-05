// Context-sensitive syntax checks retained by the permissive grammar.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "graphql/ast.hpp"

namespace graphql {

class GraphqlValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkDefinition(const ast::Definition&, cursiv::ValidationAcceptor&) const;
  void checkVariable(const ast::Variable&, cursiv::ValidationAcceptor&) const;
  void checkFragmentSpread(const ast::FragmentSpread&,
                           cursiv::ValidationAcceptor&) const;
  void checkEnumValue(const ast::EnumValueDefinition&,
                      cursiv::ValidationAcceptor&) const;
};

struct GraphqlCoreServices;
void register_validation_checks(GraphqlCoreServices& services);

}  // namespace graphql
