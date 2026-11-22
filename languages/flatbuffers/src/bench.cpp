#include "cursiv/LanguageBench.hpp"
#include "flatbuffers/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      flatbuffers::FlatbuffersSharedServices, flatbuffers::register_lsp_services>(argc, argv);
}
