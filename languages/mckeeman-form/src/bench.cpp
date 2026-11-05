#include "cursiv/LanguageBench.hpp"
#include "mckeemanform/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      mckeemanform::MckeemanFormSharedServices, mckeemanform::register_lsp_services>(argc, argv);
}
