#include "cursiv/LanguageBench.hpp"
#include "sici/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      sici::SiciSharedServices, sici::register_lsp_services>(argc, argv);
}
