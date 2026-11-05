#include "cursiv/LanguageBench.hpp"
#include "recfile/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      recfile::RecfileSharedServices, recfile::register_lsp_services>(argc, argv);
}
