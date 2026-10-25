#include "cursiv/bench/LanguageBench.hpp"
#include "ttm/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      ttm::TtmSharedServices, ttm::register_lsp_services>(argc, argv);
}
