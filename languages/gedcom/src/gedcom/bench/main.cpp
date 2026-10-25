#include "cursiv/bench/LanguageBench.hpp"
#include "gedcom/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      gedcom::GedcomSharedServices, gedcom::register_lsp_services>(argc, argv);
}
