#include "cursiv/bench/LanguageBench.hpp"
#include "csv/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      csv::CsvSharedServices, csv::register_lsp_services>(argc, argv);
}
