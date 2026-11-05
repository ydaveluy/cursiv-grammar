#include "cursiv/LanguageBench.hpp"
#include "srt/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      srt::SrtSharedServices, srt::register_lsp_services>(argc, argv);
}
