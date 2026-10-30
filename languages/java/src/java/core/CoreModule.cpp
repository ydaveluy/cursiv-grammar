// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "cursiv/core/parser/Parser.hpp"
#include "java/core/CoreServices.hpp"
#include "java/core/UnicodeEscapes.hpp"
#include "java/core/validation/JavaValidator.hpp"
#include "java/generated/Java/Module.hpp"

namespace java {

void install_core_module(JavaCoreServices& services) {
  java::generated::Java::install_generated_module(services);
  services.parser = cursiv::translating_parser(std::move(services.parser),
                                               translate_unicode_escapes);
  register_validation_checks(services);
}

}  // namespace java
