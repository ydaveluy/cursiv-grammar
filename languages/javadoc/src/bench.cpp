#include "cursiv/LanguageBench.hpp"
#include "javadoc/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      javadoc::JavadocSharedServices, javadoc::register_lsp_services>(argc, argv);
}
