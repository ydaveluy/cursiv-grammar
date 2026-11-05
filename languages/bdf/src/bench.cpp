#include "cursiv/LanguageBench.hpp"
#include "bdf/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      bdf::BdfSharedServices, bdf::register_lsp_services>(argc, argv);
}
