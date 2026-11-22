#include "cursiv/LanguageBench.hpp"
#include "janus/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      janus::JanusSharedServices, janus::register_lsp_services>(argc, argv);
}
