#include "cursiv/LanguageBench.hpp"
#include "gdscript/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      gdscript::GdscriptSharedServices, gdscript::register_lsp_services>(argc, argv);
}
