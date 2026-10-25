#include "cursiv/bench/LanguageBench.hpp"
#include "provn/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      provn::ProvNSharedServices, provn::register_lsp_services>(argc, argv);
}
