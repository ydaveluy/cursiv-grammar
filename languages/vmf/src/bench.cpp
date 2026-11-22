#include "cursiv/LanguageBench.hpp"
#include "vmf/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      vmf::VmfSharedServices, vmf::register_lsp_services>(argc, argv);
}
