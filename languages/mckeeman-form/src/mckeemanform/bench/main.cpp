#include "cursiv/bench/LanguageBench.hpp"
#include "mckeemanform/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      mckeemanform::MckeemanFormSharedServices, mckeemanform::register_lsp_services>(argc, argv);
}
