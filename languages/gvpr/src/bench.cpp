#include "cursiv/LanguageBench.hpp"
#include "gvpr/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      gvpr::GvprSharedServices, gvpr::register_lsp_services>(argc, argv);
}
