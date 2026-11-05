#include "cursiv/LanguageBench.hpp"
#include "gedcom/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      gedcom::GedcomSharedServices, gedcom::register_lsp_services>(argc, argv);
}
