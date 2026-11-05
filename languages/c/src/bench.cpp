#include "cursiv/LanguageBench.hpp"
#include "c/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      c::CSharedServices, c::register_lsp_services>(argc, argv);
}
