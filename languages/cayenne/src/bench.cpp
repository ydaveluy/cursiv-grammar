#include "cursiv/LanguageBench.hpp"
#include "cayenne/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      cayenne::CayenneSharedServices, cayenne::register_lsp_services>(argc, argv);
}
