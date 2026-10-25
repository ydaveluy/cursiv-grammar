#include "cursiv/bench/LanguageBench.hpp"
#include "tinyosnesc/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      tinyosnesc::TinyosNescSharedServices, tinyosnesc::register_lsp_services>(argc, argv);
}
