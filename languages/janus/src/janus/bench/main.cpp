#include "cursiv/bench/LanguageBench.hpp"
#include "janus/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      janus::JanusSharedServices, janus::register_lsp_services>(argc, argv);
}
