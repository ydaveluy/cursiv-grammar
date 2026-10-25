#include "cursiv/bench/LanguageBench.hpp"
#include "flatbuffers/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      flatbuffers::FlatbuffersSharedServices, flatbuffers::register_lsp_services>(argc, argv);
}
