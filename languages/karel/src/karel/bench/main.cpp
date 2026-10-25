#include "cursiv/bench/LanguageBench.hpp"
#include "karel/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      karel::KarelSharedServices, karel::register_lsp_services>(argc, argv);
}
