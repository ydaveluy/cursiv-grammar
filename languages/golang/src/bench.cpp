#include "cursiv/LanguageBench.hpp"
#include "golang/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      golang::GolangSharedServices, golang::register_lsp_services>(argc, argv);
}
