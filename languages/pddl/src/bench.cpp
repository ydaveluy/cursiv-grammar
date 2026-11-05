#include "cursiv/LanguageBench.hpp"
#include "pddl/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      pddl::PddlSharedServices, pddl::register_lsp_services>(argc, argv);
}
