#include "cursiv/bench/LanguageBench.hpp"
#include "datalog/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      datalog::DatalogSharedServices, datalog::register_lsp_services>(argc, argv);
}
