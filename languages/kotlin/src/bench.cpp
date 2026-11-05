#include "cursiv/LanguageBench.hpp"
#include "kotlin/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      kotlin::KotlinSharedServices, kotlin::register_lsp_services>(argc, argv);
}
