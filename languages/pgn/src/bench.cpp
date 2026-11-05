#include "cursiv/LanguageBench.hpp"
#include "pgn/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      pgn::PgnSharedServices, pgn::register_lsp_services>(argc, argv);
}
