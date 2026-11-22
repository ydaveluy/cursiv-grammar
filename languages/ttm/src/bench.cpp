#include "cursiv/LanguageBench.hpp"
#include "ttm/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      ttm::TtmSharedServices, ttm::register_lsp_services>(argc, argv);
}
