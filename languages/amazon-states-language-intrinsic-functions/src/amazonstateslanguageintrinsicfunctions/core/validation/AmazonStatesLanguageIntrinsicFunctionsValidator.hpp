// The one check the grammar cannot make: a call's argument count against
// the fixed arity of its intrinsic, per the States Language spec.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "amazonstateslanguageintrinsicfunctions/core/ast.hpp"

namespace amazonstateslanguageintrinsicfunctions {

class AmazonStatesLanguageIntrinsicFunctionsValidator final
    : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkArgumentCount(const ast::FuncCall& call,
                          cursiv::ValidationAcceptor& accept) const;
};

struct AmazonStatesLanguageIntrinsicFunctionsCoreServices;
void register_validation_checks(
    AmazonStatesLanguageIntrinsicFunctionsCoreServices& services);

}  // namespace amazonstateslanguageintrinsicfunctions
