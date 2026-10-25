#include "cursiv/bench/LanguageBench.hpp"
#include "informix/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      informix::InformixSharedServices, informix::register_lsp_services>(argc, argv);
}
