#include "cursiv/bench/LanguageBench.hpp"
#include "cayenne/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      cayenne::CayenneSharedServices, cayenne::register_lsp_services>(argc, argv);
}
