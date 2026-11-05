#include "cursiv/LanguageBench.hpp"
#include "pascal/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      pascal::PascalSharedServices, pascal::register_lsp_services>(argc, argv);
}
