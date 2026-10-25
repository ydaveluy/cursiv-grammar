#include "cursiv/bench/LanguageBench.hpp"
#include "thrift/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      thrift::ThriftSharedServices, thrift::register_lsp_services>(argc, argv);
}
