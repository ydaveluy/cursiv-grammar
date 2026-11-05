#include "cursiv/LanguageBench.hpp"
#include "mps/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      mps::MpsSharedServices, mps::register_lsp_services>(argc, argv);
}
