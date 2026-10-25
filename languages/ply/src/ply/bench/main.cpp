#include "cursiv/bench/LanguageBench.hpp"
#include "ply/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      ply::PlySharedServices, ply::register_lsp_services>(argc, argv);
}
