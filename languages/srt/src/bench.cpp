#include "cursiv/LanguageBench.hpp"
#include "srt/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      srt::SrtSharedServices, srt::register_lsp_services>(argc, argv);
}
