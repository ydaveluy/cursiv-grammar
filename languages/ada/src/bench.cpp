#include "cursiv/LanguageBench.hpp"
#include "ada/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      ada::AdaSharedServices, ada::register_lsp_services>(argc, argv);
}
