#include "cursiv/LanguageBench.hpp"
#include "dcm/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      dcm::DcmSharedServices, dcm::register_lsp_services>(argc, argv);
}
