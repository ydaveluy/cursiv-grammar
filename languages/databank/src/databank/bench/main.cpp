#include "cursiv/bench/LanguageBench.hpp"
#include "databank/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      databank::DatabankSharedServices, databank::register_lsp_services>(argc, argv);
}
