#include "cursiv/LanguageBench.hpp"
#include "sharc/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      sharc::SharcSharedServices, sharc::register_lsp_services>(argc, argv);
}
