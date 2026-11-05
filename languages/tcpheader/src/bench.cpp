#include "cursiv/LanguageBench.hpp"
#include "tcpheader/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      tcpheader::TcpheaderSharedServices, tcpheader::register_lsp_services>(argc, argv);
}
