#include "cursiv/LanguageBench.hpp"
#include "rfc822/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      rfc822::Rfc822SharedServices, rfc822::register_lsp_services>(argc, argv);
}
