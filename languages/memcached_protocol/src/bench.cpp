#include "cursiv/LanguageBench.hpp"
#include "memcachedprotocol/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      memcachedprotocol::MemcachedProtocolSharedServices, memcachedprotocol::register_lsp_services>(argc, argv);
}
