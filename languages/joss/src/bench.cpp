#include "cursiv/LanguageBench.hpp"
#include "joss/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      joss::JossSharedServices, joss::register_lsp_services>(argc, argv);
}
