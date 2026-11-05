#include "cursiv/LanguageBench.hpp"
#include "caql/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      caql::CaqlSharedServices, caql::register_lsp_services>(argc, argv);
}
