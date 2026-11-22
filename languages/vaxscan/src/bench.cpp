#include "cursiv/LanguageBench.hpp"
#include "vaxscan/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      vaxscan::VaxscanSharedServices, vaxscan::register_lsp_services>(argc, argv);
}
