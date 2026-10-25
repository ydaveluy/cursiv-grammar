#include "cursiv/bench/LanguageBench.hpp"
#include "cobol85/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      cobol85::Cobol85SharedServices, cobol85::register_lsp_services>(argc, argv);
}
