#include "cursiv/LanguageBench.hpp"
#include "sqlite/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      sqlite::SqliteSharedServices, sqlite::register_lsp_services>(argc, argv);
}
