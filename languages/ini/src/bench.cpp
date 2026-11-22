#include "cursiv/LanguageBench.hpp"
#include "ini/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      ini::IniSharedServices, ini::register_lsp_services>(argc, argv);
}
