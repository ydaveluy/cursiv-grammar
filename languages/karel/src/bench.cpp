#include "cursiv/LanguageBench.hpp"
#include "karel/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      karel::KarelSharedServices, karel::register_lsp_services>(argc, argv);
}
