#include "cursiv/LanguageBench.hpp"
#include "webidl/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      webidl::WebidlSharedServices, webidl::register_lsp_services>(argc, argv);
}
