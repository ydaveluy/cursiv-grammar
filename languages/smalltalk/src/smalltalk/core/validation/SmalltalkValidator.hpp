// Local AST predicates used by the pinned Amber parser.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "smalltalk/core/ast.hpp"

namespace smalltalk {

class SmalltalkValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkDictionary(const ast::DynamicDictionary&,
                       cursiv::ValidationAcceptor&) const;
  void checkCascade(const ast::Cascade&, cursiv::ValidationAcceptor&) const;
};

struct SmalltalkCoreServices;
void register_validation_checks(SmalltalkCoreServices& services);

}  // namespace smalltalk
