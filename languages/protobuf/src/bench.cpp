#include "cursiv/LanguageBench.hpp"
#include "protobuf/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      protobuf::ProtobufSharedServices, protobuf::register_lsp_services>(argc, argv);
}
