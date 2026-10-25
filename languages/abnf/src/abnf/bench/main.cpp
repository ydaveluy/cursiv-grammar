#include "cursiv/bench/LanguageBench.hpp"
#include "abnf/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      abnf::AbnfSharedServices, abnf::register_lsp_services>(argc, argv);
}
