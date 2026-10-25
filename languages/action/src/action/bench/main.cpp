#include "cursiv/bench/LanguageBench.hpp"
#include "action/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      action::ActionSharedServices, action::register_lsp_services>(argc, argv);
}
