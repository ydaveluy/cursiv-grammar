#include "cursiv/LanguageBench.hpp"
#include "nasm/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      nasm::NasmSharedServices, nasm::register_lsp_services>(argc, argv);
}
