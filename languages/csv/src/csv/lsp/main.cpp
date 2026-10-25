// The csv language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/lsp/runtime/LanguageServerMain.hpp"
#include "csv/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      csv::CsvSharedServices>(
      argc, argv, "csv-lsp", &csv::register_lsp_services);
}
