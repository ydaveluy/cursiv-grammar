#include "cursiv/LanguageBench.hpp"
#include "janus/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      janus::JanusSharedServices, janus::register_lsp_services>(argc, argv);
}
