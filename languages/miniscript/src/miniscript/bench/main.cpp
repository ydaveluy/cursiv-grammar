#include "cursiv/bench/LanguageBench.hpp"
#include "miniscript/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      miniscript::MiniscriptSharedServices, miniscript::register_lsp_services>(argc, argv);
}
