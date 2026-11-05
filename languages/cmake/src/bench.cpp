#include "cursiv/LanguageBench.hpp"
#include "cmake/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      cmake::CmakeSharedServices, cmake::register_lsp_services>(argc, argv);
}
