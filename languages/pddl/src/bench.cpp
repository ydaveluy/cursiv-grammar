#include "cursiv/LanguageBench.hpp"
#include "pddl/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      pddl::PddlSharedServices, pddl::register_lsp_services>(argc, argv);
}
