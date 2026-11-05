#include "cursiv/LanguageBench.hpp"
#include "antlr/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      antlr::AntlrSharedServices, antlr::register_lsp_services>(argc, argv);
}
