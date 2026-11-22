#include "cursiv/LanguageBench.hpp"
#include "hypertalk/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      hypertalk::HypertalkSharedServices, hypertalk::register_lsp_services>(argc, argv);
}
