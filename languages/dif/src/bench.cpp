#include "cursiv/LanguageBench.hpp"
#include "dif/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      dif::DifSharedServices, dif::register_lsp_services>(argc, argv);
}
