#include "cursiv/LanguageBench.hpp"
#include "lua/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      lua::LuaSharedServices, lua::register_lsp_services>(argc, argv);
}
