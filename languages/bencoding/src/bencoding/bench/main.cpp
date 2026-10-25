#include "cursiv/bench/LanguageBench.hpp"
#include "bencoding/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      bencoding::BencodingSharedServices, bencoding::register_lsp_services>(argc, argv);
}
