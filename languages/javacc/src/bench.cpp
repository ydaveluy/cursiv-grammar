#include "cursiv/LanguageBench.hpp"
#include "javacc/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      javacc::JavaccSharedServices, javacc::register_lsp_services>(argc, argv);
}
