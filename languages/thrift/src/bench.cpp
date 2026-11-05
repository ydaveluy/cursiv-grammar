#include "cursiv/LanguageBench.hpp"
#include "thrift/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      thrift::ThriftSharedServices, thrift::register_lsp_services>(argc, argv);
}
