#include "cursiv/bench/LanguageBench.hpp"
#include "angelscript/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      angelscript::AngelscriptSharedServices, angelscript::register_lsp_services>(argc, argv);
}
