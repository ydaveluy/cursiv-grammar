#include "powerbuilder/core/validation/PowerbuilderValidator.hpp"

#include <memory>
#include <charconv>
#include <system_error>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "powerbuilder/core/CoreServices.hpp"

namespace powerbuilder {

void PowerbuilderValidator::checkNumber(const ast::NumberLit& node,
                                      cursiv::ValidationAcceptor& accept) const {
  std::string_view text=node.value;
  const bool suffix=!text.empty() && (text.back()=='d'||text.back()=='D'||text.back()=='f'||text.back()=='F');
  if(suffix) text.remove_suffix(1);
  bool invalid=false;
  if(suffix || text.find_first_of("eE")!=std::string_view::npos) {
    double value{};
    const auto r=std::from_chars(text.data(),text.data()+text.size(),value);
    invalid=r.ec!=std::errc{}||r.ptr!=text.data()+text.size();
  } else if(text.find('.')==std::string_view::npos) {
    const auto first=text.find_first_not_of('0');
    text=first==std::string_view::npos?std::string_view("0"):text.substr(first);
    const auto* unary=cursiv::ast_cast<ast::UnaryExpr>(node.parent);
    const std::string_view max=unary&&unary->op=="-" ? "9223372036854775808" : "9223372036854775807";
    invalid=text.size()>max.size()||(text.size()==max.size()&&text>max);
  }
  if(invalid)
    accept.error(node,"Numeric literal is outside its native range.","powerbuilder.number-range")
        .property<&ast::NumberLit::value>();
}

void register_validation_checks(PowerbuilderCoreServices& services) {
  services.validator = std::make_unique<const PowerbuilderValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&PowerbuilderValidator::checkNumber>(*services.validator)});
}

}  // namespace powerbuilder
