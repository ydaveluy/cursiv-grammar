#include "cursiv/LanguageBench.hpp"
#include "stacktrace/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      stacktrace::StacktraceSharedServices, stacktrace::register_lsp_services>(argc, argv);
}
