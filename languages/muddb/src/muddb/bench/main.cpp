#include "cursiv/bench/LanguageBench.hpp"
#include "muddb/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      muddb::MuddbSharedServices, muddb::register_lsp_services>(argc, argv);
}
