#include "cursiv/bench/LanguageBench.hpp"
#include "tcpheader/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      tcpheader::TcpheaderSharedServices, tcpheader::register_lsp_services>(argc, argv);
}
