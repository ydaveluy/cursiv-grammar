#include "cursiv/LanguageBench.hpp"
#include "fortran/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      fortran::FortranSharedServices, fortran::register_lsp_services>(argc, argv);
}
