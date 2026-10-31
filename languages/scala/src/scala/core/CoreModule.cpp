// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include <utility>

#include "cursiv/core/parser/Parser.hpp"
#include "scala/core/CoreServices.hpp"
#include "scala/core/UnicodeSource.hpp"
#include "scala/core/validation/ScalaValidator.hpp"
#include "scala/generated/Scala/Module.hpp"

namespace scala {

void install_core_module(ScalaCoreServices& services) {
  scala::generated::Scala::install_generated_module(services);
  services.parser = cursiv::translating_parser(
      std::move(services.parser), translate_unicode_source);
  register_validation_checks(services);
}

}  // namespace scala
