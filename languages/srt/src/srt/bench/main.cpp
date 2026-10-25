#include "cursiv/bench/LanguageBench.hpp"
#include "srt/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      srt::SrtSharedServices, srt::register_lsp_services>(argc, argv);
}
