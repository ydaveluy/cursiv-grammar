#include "cursiv/LanguageBench.hpp"
#include "star/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      star::StarSharedServices, star::register_lsp_services>(argc, argv);
}
