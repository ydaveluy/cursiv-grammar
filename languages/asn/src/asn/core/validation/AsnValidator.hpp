// Local tag and REAL checks, without resolving imported or defined types.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "asn/core/ast.hpp"

namespace asn {

class AsnValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkTag(const ast::TaggedType&, cursiv::ValidationAcceptor&) const;
  void checkValueAssignment(const ast::ValueAssignment&,
                            cursiv::ValidationAcceptor&) const;
  void checkComponent(const ast::Component&, cursiv::ValidationAcceptor&) const;
  void checkFieldSpec(const ast::FieldSpec&, cursiv::ValidationAcceptor&) const;

 private:
  void checkReal(const ast::Type*, const ast::Value*,
                 cursiv::ValidationAcceptor&) const;
};

struct AsnCoreServices;
void register_validation_checks(AsnCoreServices& services);

}  // namespace asn
