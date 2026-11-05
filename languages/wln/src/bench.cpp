#include "cursiv/LanguageBench.hpp"
#include "wln/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      wln::WlnSharedServices, wln::register_lsp_services>(argc, argv);
}
