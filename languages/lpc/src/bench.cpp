#include "cursiv/LanguageBench.hpp"
#include "lpc/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      lpc::LpcSharedServices, lpc::register_lsp_services>(argc, argv);
}
