#include "cursiv/LanguageBench.hpp"
#include "sgf/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      sgf::SgfSharedServices, sgf::register_lsp_services>(argc, argv);
}
