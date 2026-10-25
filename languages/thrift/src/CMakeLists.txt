# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `thriftGenerated` (the compiled library) and `thrift_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID thrift
    GRAMMAR    thrift/core/ThriftGrammar.hpp
    NAMESPACE  thrift::Thrift
    EXTENSIONS .thrift
    OUT        thrift/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/thrift/core/*.cpp)
add_library(thriftCore ${_core})
target_link_libraries(thriftCore PUBLIC thriftGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/thrift/cli/*.cpp)
add_executable(thrift_cli ${_cli})
target_link_libraries(thrift_cli
                      PRIVATE thriftCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/thrift/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/thrift/lsp/main.cpp)
add_library(thriftLsp ${_lsp})
target_link_libraries(thriftLsp PUBLIC thriftCore cursiv::lsp)
add_executable(thrift_lsp thrift/lsp/main.cpp)
target_link_libraries(thrift_lsp PRIVATE thriftLsp)

cursiv_add_benchmark(thrift SOURCES thrift/bench/main.cpp)

cursiv_package_language(thrift)
