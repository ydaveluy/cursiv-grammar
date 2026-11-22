#include "cursiv/LanguageBench.hpp"
#include "stl/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      stl::StlSharedServices, stl::register_lsp_services>(argc, argv);
}
