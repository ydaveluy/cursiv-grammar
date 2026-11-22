#include "cursiv/LanguageBench.hpp"
#include "star/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      star::StarSharedServices, star::register_lsp_services>(argc, argv);
}
