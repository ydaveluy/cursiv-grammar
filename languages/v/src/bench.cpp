#include "cursiv/LanguageBench.hpp"
#include "v/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      v::VSharedServices, v::register_lsp_services>(argc, argv);
}
