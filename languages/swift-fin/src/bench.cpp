#include "cursiv/LanguageBench.hpp"
#include "swiftfin/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      swiftfin::SwiftFinSharedServices, swiftfin::register_lsp_services>(argc, argv);
}
