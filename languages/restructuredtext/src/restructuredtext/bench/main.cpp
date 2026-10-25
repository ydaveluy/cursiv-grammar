#include "cursiv/bench/LanguageBench.hpp"
#include "restructuredtext/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      restructuredtext::RestructuredtextSharedServices, restructuredtext::register_lsp_services>(argc, argv);
}
