#include "cursiv/bench/LanguageBench.hpp"
#include "sgf/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      sgf::SgfSharedServices, sgf::register_lsp_services>(argc, argv);
}
