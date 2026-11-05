// Object-member checks run after permissive field-list parsing.
#pragma once

#include "amazonstateslanguage/ast.hpp"
#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"

namespace amazonstateslanguage {

class AmazonStatesLanguageValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  template <class T>
  void checkMembers(const T &, cursiv::ValidationAcceptor &) const;
  void checkProgram(const ast::Program&, cursiv::ValidationAcceptor&) const;
  void checkState(const ast::State&, cursiv::ValidationAcceptor&) const;
};

struct AmazonStatesLanguageCoreServices;
void register_validation_checks(AmazonStatesLanguageCoreServices& services);

}  // namespace amazonstateslanguage
