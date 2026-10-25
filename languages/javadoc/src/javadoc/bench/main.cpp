#include "cursiv/bench/LanguageBench.hpp"
#include "javadoc/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      javadoc::JavadocSharedServices, javadoc::register_lsp_services>(argc, argv);
}
