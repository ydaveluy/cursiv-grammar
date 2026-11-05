// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "cursiv/parser/Parser.hpp"
#include "sparql/CodepointEscapes.hpp"
#include "sparql/CoreServices.hpp"
#include "sparql/validation/SparqlValidator.hpp"
#include "sparql/generated/Sparql/Module.hpp"

namespace sparql {

void install_core_module(SparqlCoreServices& services) {
  sparql::generated::Sparql::install_generated_module(services);
  services.parser = cursiv::translating_parser(std::move(services.parser),
                                               translate_codepoint_escapes);
  register_validation_checks(services);
}

}  // namespace sparql
