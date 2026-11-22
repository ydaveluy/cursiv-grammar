#include "cursiv/LanguageBench.hpp"
#include "action/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      action::ActionSharedServices, action::register_lsp_services>(argc, argv);
}
