#include "cursiv/LanguageBench.hpp"
#include "parkingsign/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      parkingsign::ParkingsignSharedServices, parkingsign::register_lsp_services>(argc, argv);
}
