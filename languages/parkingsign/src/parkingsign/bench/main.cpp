#include "cursiv/bench/LanguageBench.hpp"
#include "parkingsign/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      parkingsign::ParkingsignSharedServices, parkingsign::register_lsp_services>(argc, argv);
}
