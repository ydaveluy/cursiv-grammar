#include "cursiv/LanguageBench.hpp"
#include "tcpheader/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      tcpheader::TcpheaderSharedServices, tcpheader::register_lsp_services>(argc, argv);
}
