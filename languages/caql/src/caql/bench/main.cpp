#include "cursiv/bench/LanguageBench.hpp"
#include "caql/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      caql::CaqlSharedServices, caql::register_lsp_services>(argc, argv);
}
