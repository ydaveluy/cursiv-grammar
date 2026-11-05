#include "cursiv/LanguageBench.hpp"
#include "telephone/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      telephone::TelephoneSharedServices, telephone::register_lsp_services>(argc, argv);
}
