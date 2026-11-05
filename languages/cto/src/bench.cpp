#include "cursiv/LanguageBench.hpp"
#include "cto/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      cto::CtoSharedServices, cto::register_lsp_services>(argc, argv);
}
