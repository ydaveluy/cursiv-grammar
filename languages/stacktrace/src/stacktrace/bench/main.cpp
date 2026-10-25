#include "cursiv/bench/LanguageBench.hpp"
#include "stacktrace/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      stacktrace::StacktraceSharedServices, stacktrace::register_lsp_services>(argc, argv);
}
