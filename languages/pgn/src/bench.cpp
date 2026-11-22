#include "cursiv/LanguageBench.hpp"
#include "pgn/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      pgn::PgnSharedServices, pgn::register_lsp_services>(argc, argv);
}
