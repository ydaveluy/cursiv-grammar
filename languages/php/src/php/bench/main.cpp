#include "cursiv/bench/LanguageBench.hpp"
#include "php/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      php::PhpSharedServices, php::register_lsp_services>(argc, argv);
}
