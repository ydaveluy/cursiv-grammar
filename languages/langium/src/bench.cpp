#include "cursiv/LanguageBench.hpp"
#include "langium/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      langium::LangiumSharedServices, langium::register_lsp_services>(argc, argv);
}
