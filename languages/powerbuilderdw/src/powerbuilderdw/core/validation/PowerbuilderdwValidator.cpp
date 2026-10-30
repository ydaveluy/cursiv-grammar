#include "powerbuilderdw/core/validation/PowerbuilderdwValidator.hpp"

#include <memory>
#include <charconv>
#include <string>
#include <system_error>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "powerbuilderdw/core/CoreServices.hpp"

namespace powerbuilderdw {

void PowerbuilderdwValidator::checkNumber(const ast::NumberValue& node,
                                         cursiv::ValidationAcceptor& accept) const {
  if(node.text.find_first_of("eEfF")==std::string_view::npos)return;
  std::string text;
  for(char c:node.text)if(c!=' '&&c!='\t'&&c!='\r'&&c!='\n')text+=c;
  if(!text.empty()&&(text.back()=='f'||text.back()=='F'||text.back()=='d'||text.back()=='D'))text.pop_back();
  std::string_view number=text;
  if(number.starts_with('+'))number.remove_prefix(1);
  double value{};
  const auto r=std::from_chars(number.data(),number.data()+number.size(),value);
  if(r.ec!=std::errc{}||r.ptr!=number.data()+number.size())
    accept.error(node,"Floating-point literal is outside its range.","powerbuilderdw.number-range")
        .property<&ast::NumberValue::text>();
}

void register_validation_checks(PowerbuilderdwCoreServices& services) {
  using V = PowerbuilderdwValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkNumber>(*services.validator)});
}

}  // namespace powerbuilderdw
