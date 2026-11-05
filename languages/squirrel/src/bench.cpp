#include "cursiv/LanguageBench.hpp"
#include "squirrel/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      squirrel::SquirrelSharedServices, squirrel::register_lsp_services>(argc, argv);
}
