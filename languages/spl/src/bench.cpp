#include "cursiv/LanguageBench.hpp"
#include "spl/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      spl::SplSharedServices, spl::register_lsp_services>(argc, argv);
}
