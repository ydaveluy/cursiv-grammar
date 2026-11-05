#include "cursiv/LanguageBench.hpp"
#include "informix/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      informix::InformixSharedServices, informix::register_lsp_services>(argc, argv);
}
