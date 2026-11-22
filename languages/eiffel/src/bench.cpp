#include "cursiv/LanguageBench.hpp"
#include "eiffel/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      eiffel::EiffelSharedServices, eiffel::register_lsp_services>(argc, argv);
}
