// Object-member checks run after permissive field-list parsing.
#pragma once

#include "amazonstateslanguage/core/ast.hpp"
#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"

namespace amazonstateslanguage {

class AmazonStatesLanguageValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  template <class T>
  void checkMembers(const T &, cursiv::ValidationAcceptor &) const;
};

struct AmazonStatesLanguageCoreServices;
void register_validation_checks(AmazonStatesLanguageCoreServices& services);

}  // namespace amazonstateslanguage
