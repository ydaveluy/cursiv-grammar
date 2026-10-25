#include "cursiv/bench/LanguageBench.hpp"
#include "sdl/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      sdl::SdlSharedServices, sdl::register_lsp_services>(argc, argv);
}
