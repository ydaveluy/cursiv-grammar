#include "cursiv/bench/LanguageBench.hpp"
#include "desktopentry/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      desktopentry::DesktopEntrySharedServices, desktopentry::register_lsp_services>(argc, argv);
}
