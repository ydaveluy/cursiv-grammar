// The checks of this language: what the grammar reads but C refuses. A
// reserved word is one -- the grammar takes it for a name wherever the state
// reads names alone, and refuses it only as a declarator's name.
#pragma once

#include "c/core/ast.hpp"
#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"

namespace c {

class CValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  // An identifier spelling a reserved word, anywhere in the unit.
  void checkReservedNames(const ast::TranslationUnit& unit,
                          cursiv::ValidationAcceptor& accept) const;
  void checkInference(const ast::Specifiers& node,
                      cursiv::ValidationAcceptor& accept) const;
  void checkEnumBase(const ast::EnumType& type,
                     cursiv::ValidationAcceptor& accept) const;
};

struct CCoreServices;
void register_validation_checks(CCoreServices& services);

}  // namespace c
