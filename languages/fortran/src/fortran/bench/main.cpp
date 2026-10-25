#include "cursiv/bench/LanguageBench.hpp"
#include "fortran/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      fortran::FortranSharedServices, fortran::register_lsp_services>(argc, argv);
}
