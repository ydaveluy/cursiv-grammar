#include "cursiv/bench/LanguageBench.hpp"
#include "rexx/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      rexx::RexxSharedServices, rexx::register_lsp_services>(argc, argv);
}
