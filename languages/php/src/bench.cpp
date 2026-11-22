#include "cursiv/LanguageBench.hpp"
#include "php/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      php::PhpSharedServices, php::register_lsp_services>(argc, argv);
}
