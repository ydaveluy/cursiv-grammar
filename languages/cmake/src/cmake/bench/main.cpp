#include "cursiv/bench/LanguageBench.hpp"
#include "cmake/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      cmake::CmakeSharedServices, cmake::register_lsp_services>(argc, argv);
}
