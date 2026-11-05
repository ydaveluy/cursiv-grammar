#include "cursiv/LanguageBench.hpp"
#include "refal/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      refal::RefalSharedServices, refal::register_lsp_services>(argc, argv);
}
