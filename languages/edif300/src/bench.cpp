#include "cursiv/LanguageBench.hpp"
#include "edif300/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      edif300::Edif300SharedServices, edif300::register_lsp_services>(argc, argv);
}
