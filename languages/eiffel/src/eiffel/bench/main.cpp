#include "cursiv/bench/LanguageBench.hpp"
#include "eiffel/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      eiffel::EiffelSharedServices, eiffel::register_lsp_services>(argc, argv);
}
