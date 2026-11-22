#include "cursiv/LanguageBench.hpp"
#include "kuka/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      kuka::KukaSharedServices, kuka::register_lsp_services>(argc, argv);
}
