#include "cursiv/LanguageBench.hpp"
#include "csv/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      csv::CsvSharedServices, csv::register_lsp_services>(argc, argv);
}
