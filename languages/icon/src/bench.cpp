#include "cursiv/LanguageBench.hpp"
#include "icon/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      icon::IconSharedServices, icon::register_lsp_services>(argc, argv);
}
