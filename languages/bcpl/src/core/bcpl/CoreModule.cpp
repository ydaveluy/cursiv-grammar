// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "bcpl/CoreServices.hpp"
#include "bcpl/SectionTags.hpp"
#include "bcpl/validation/BcplValidator.hpp"
#include "bcpl/generated/Bcpl/Module.hpp"
#include "cursiv/parser/Parser.hpp"

namespace bcpl {

void install_core_module(BcplCoreServices& services) {
  bcpl::generated::Bcpl::installGeneratedModule(services);
  services.parser = cursiv::translatingParser(std::move(services.parser),
                                               translate_section_tags);
  register_validation_checks(services);
}

}  // namespace bcpl
