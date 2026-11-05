#include "cursiv/LanguageBench.hpp"
#include "szf/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      szf::SzfSharedServices, szf::register_lsp_services>(argc, argv);
}
