#include "cursiv/LanguageBench.hpp"
#include "dart2/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      dart2::Dart2SharedServices, dart2::register_lsp_services>(argc, argv);
}
