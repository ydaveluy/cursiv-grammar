#include "cursiv/bench/LanguageBench.hpp"
#include "squirrel/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      squirrel::SquirrelSharedServices, squirrel::register_lsp_services>(argc, argv);
}
