#include "cursiv/LanguageBench.hpp"
#include "abnf/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      abnf::AbnfSharedServices, abnf::register_lsp_services>(argc, argv);
}
