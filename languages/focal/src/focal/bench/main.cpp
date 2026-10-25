#include "cursiv/bench/LanguageBench.hpp"
#include "focal/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      focal::FocalSharedServices, focal::register_lsp_services>(argc, argv);
}
