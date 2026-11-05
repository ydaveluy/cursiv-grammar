#include "cursiv/LanguageBench.hpp"
#include "provn/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      provn::ProvNSharedServices, provn::register_lsp_services>(argc, argv);
}
