#include "cursiv/LanguageBench.hpp"
#include "tinyosnesc/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      tinyosnesc::TinyosNescSharedServices, tinyosnesc::register_lsp_services>(argc, argv);
}
