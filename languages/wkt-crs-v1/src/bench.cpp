#include "cursiv/LanguageBench.hpp"
#include "wktcrsv1/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      wktcrsv1::WktCrsV1SharedServices, wktcrsv1::register_lsp_services>(argc, argv);
}
