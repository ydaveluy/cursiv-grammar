#include "cursiv/bench/LanguageBench.hpp"
#include "ini/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      ini::IniSharedServices, ini::register_lsp_services>(argc, argv);
}
