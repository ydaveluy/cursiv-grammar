#include "cursiv/LanguageBench.hpp"
#include "arden/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      arden::ArdenSharedServices, arden::register_lsp_services>(argc, argv);
}
