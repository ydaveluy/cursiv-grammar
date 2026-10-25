#include "cursiv/bench/LanguageBench.hpp"
#include "kuka/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      kuka::KukaSharedServices, kuka::register_lsp_services>(argc, argv);
}
