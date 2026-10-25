# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `callableGenerated` (the compiled library) and `callable_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID callable
    GRAMMAR    callable/core/CallableGrammar.hpp
    NAMESPACE  callable::Callable
    EXTENSIONS .callable
    OUT        callable/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/callable/core/*.cpp)
add_library(callableCore ${_core})
target_link_libraries(callableCore PUBLIC callableGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/callable/cli/*.cpp)
add_executable(callable_cli ${_cli})
target_link_libraries(callable_cli
                      PRIVATE callableCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/callable/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/callable/lsp/main.cpp)
add_library(callableLsp ${_lsp})
target_link_libraries(callableLsp PUBLIC callableCore cursiv::lsp)
add_executable(callable_lsp callable/lsp/main.cpp)
target_link_libraries(callable_lsp PRIVATE callableLsp)

cursiv_add_benchmark(callable SOURCES callable/bench/main.cpp)

cursiv_package_language(callable)
