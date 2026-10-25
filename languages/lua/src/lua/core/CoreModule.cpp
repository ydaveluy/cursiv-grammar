// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "lua/core/CoreServices.hpp"
#include "lua/core/validation/LuaValidator.hpp"
#include "lua/generated/Lua/Module.hpp"

namespace lua {

void install_core_module(LuaCoreServices& services) {
  lua::generated::Lua::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace lua
