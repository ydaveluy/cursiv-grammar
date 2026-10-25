// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "sqlite/core/CoreServices.hpp"
#include "sqlite/core/validation/SqliteValidator.hpp"
#include "sqlite/generated/Sqlite/Module.hpp"

namespace sqlite {

void install_core_module(SqliteCoreServices& services) {
  sqlite::generated::Sqlite::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace sqlite
