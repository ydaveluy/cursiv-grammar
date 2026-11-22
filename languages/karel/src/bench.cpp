#include "cursiv/LanguageBench.hpp"
#include "karel/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      karel::KarelSharedServices, karel::register_lsp_services>(argc, argv);
}
