#include "cursiv/LanguageBench.hpp"
#include "atl/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      atl::AtlSharedServices, atl::register_lsp_services>(argc, argv);
}
