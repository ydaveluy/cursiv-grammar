#pragma once
#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "xsdregex/ast.hpp"
namespace xsdregex {
class XsdRegexValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkProperty(const ast::CharProp &, cursiv::ValidationAcceptor &) const;
  void checkRange(const ast::CharRange &, cursiv::ValidationAcceptor &) const;
  void checkClass(const ast::CharClassExpr &,
                  cursiv::ValidationAcceptor &) const;
  void checkPiece(const ast::Piece &, cursiv::ValidationAcceptor &) const;
  void checkQuantifier(const ast::Quantifier &,
                       cursiv::ValidationAcceptor &) const;
};
struct XsdRegexCoreServices;
void register_validation_checks(XsdRegexCoreServices &services);
} // namespace xsdregex
