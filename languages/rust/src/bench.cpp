#include "cursiv/LanguageBench.hpp"
#include "rust/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      rust::RustSharedServices, rust::register_lsp_services>(argc, argv);
}
