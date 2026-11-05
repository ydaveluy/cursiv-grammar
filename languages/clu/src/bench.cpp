#include "cursiv/LanguageBench.hpp"
#include "clu/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      clu::CluSharedServices, clu::register_lsp_services>(argc, argv);
}
