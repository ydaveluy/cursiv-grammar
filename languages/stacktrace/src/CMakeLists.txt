# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `stacktraceGenerated` (the compiled library) and `stacktrace_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID stacktrace
    GRAMMAR    stacktrace/core/StacktraceGrammar.hpp
    NAMESPACE  stacktrace::Stacktrace
    EXTENSIONS .stacktrace
    OUT        stacktrace/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/stacktrace/core/*.cpp)
add_library(stacktraceCore ${_core})
target_link_libraries(stacktraceCore PUBLIC stacktraceGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/stacktrace/cli/*.cpp)
add_executable(stacktrace_cli ${_cli})
target_link_libraries(stacktrace_cli
                      PRIVATE stacktraceCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/stacktrace/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/stacktrace/lsp/main.cpp)
add_library(stacktraceLsp ${_lsp})
target_link_libraries(stacktraceLsp PUBLIC stacktraceCore cursiv::lsp)
add_executable(stacktrace_lsp stacktrace/lsp/main.cpp)
target_link_libraries(stacktrace_lsp PRIVATE stacktraceLsp)

cursiv_add_benchmark(stacktrace SOURCES stacktrace/bench/main.cpp)

cursiv_package_language(stacktrace)
