#include "cursiv/LanguageBench.hpp"
#include "provn/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      provn::ProvNSharedServices, provn::register_lsp_services>(argc, argv);
}
