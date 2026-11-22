// The csv language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "csv/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      csv::CsvSharedServices>(
      argc, argv, "csv-lsp", &csv::register_lsp_services);
}
