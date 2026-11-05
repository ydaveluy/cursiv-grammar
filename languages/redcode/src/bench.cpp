#include "cursiv/LanguageBench.hpp"
#include "redcode/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      redcode::RedcodeSharedServices, redcode::register_lsp_services>(argc, argv);
}
