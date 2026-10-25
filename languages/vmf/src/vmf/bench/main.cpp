#include "cursiv/bench/LanguageBench.hpp"
#include "vmf/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      vmf::VmfSharedServices, vmf::register_lsp_services>(argc, argv);
}
