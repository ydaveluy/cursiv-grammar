#include "cursiv/LanguageBench.hpp"
#include "java/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      java::JavaSharedServices, java::register_lsp_services>(argc, argv);
}
