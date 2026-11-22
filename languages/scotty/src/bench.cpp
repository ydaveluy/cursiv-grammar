#include "cursiv/LanguageBench.hpp"
#include "scotty/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      scotty::ScottySharedServices, scotty::register_lsp_services>(argc, argv);
}
