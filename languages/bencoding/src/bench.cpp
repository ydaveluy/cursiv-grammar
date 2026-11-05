#include "cursiv/LanguageBench.hpp"
#include "bencoding/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      bencoding::BencodingSharedServices, bencoding::register_lsp_services>(argc, argv);
}
