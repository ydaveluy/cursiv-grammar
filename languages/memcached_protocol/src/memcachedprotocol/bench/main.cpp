#include "cursiv/bench/LanguageBench.hpp"
#include "memcachedprotocol/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      memcachedprotocol::MemcachedProtocolSharedServices, memcachedprotocol::register_lsp_services>(argc, argv);
}
