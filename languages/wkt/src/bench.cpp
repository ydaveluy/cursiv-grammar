#include "cursiv/LanguageBench.hpp"
#include "wkt/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      wkt::WktSharedServices, wkt::register_lsp_services>(argc, argv);
}
