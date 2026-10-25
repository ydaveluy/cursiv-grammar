#include "cursiv/bench/LanguageBench.hpp"
#include "protobuf/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      protobuf::ProtobufSharedServices, protobuf::register_lsp_services>(argc, argv);
}
