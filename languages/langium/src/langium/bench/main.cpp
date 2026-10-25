#include "cursiv/bench/LanguageBench.hpp"
#include "langium/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      langium::LangiumSharedServices, langium::register_lsp_services>(argc, argv);
}
