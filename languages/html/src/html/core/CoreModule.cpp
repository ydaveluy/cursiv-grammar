// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "html/core/CoreServices.hpp"
#include "html/core/validation/HtmlValidator.hpp"
#include "html/generated/Html/Module.hpp"

namespace html {

void install_core_module(HtmlCoreServices& services) {
  html::generated::Html::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace html
