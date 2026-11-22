#include "cursiv/LanguageBench.hpp"
#include "desktopentry/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      desktopentry::DesktopEntrySharedServices, desktopentry::register_lsp_services>(argc, argv);
}
