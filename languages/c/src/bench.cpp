#include "cursiv/LanguageBench.hpp"
#include "c/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      c::CSharedServices, c::register_lsp_services>(argc, argv);
}
