#include "cursiv/bench/LanguageBench.hpp"
#include "itn/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      itn::ItnSharedServices, itn::register_lsp_services>(argc, argv);
}
