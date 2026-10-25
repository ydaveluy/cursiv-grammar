#include "cursiv/bench/LanguageBench.hpp"
#include "css3/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      css3::Css3SharedServices, css3::register_lsp_services>(argc, argv);
}
