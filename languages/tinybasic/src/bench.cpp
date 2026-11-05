#include "cursiv/LanguageBench.hpp"
#include "tinybasic/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      tinybasic::TinybasicSharedServices, tinybasic::register_lsp_services>(argc, argv);
}
