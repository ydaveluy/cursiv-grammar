#include "cursiv/bench/LanguageBench.hpp"
#include "rust/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      rust::RustSharedServices, rust::register_lsp_services>(argc, argv);
}
