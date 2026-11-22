#include "cursiv/LanguageBench.hpp"
#include "parkingsign/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      parkingsign::ParkingsignSharedServices, parkingsign::register_lsp_services>(argc, argv);
}
