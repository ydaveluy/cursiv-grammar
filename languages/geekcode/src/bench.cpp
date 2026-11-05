#include "cursiv/LanguageBench.hpp"
#include "geekcode/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      geekcode::GeekcodeSharedServices, geekcode::register_lsp_services>(argc, argv);
}
