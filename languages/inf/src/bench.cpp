#include "cursiv/LanguageBench.hpp"
#include "inf/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      inf::InfSharedServices, inf::register_lsp_services>(argc, argv);
}
