#include "cursiv/bench/LanguageBench.hpp"
#include "java/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      java::JavaSharedServices, java::register_lsp_services>(argc, argv);
}
