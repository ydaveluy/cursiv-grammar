// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "csv/core/CoreServices.hpp"
#include "csv/core/validation/CsvValidator.hpp"
#include "csv/generated/Csv/Module.hpp"

namespace csv {

void install_core_module(CsvCoreServices& services) {
  csv::generated::Csv::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace csv
