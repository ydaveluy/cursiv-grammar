# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `useragentGenerated` (the compiled library) and `useragent_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID useragent
    GRAMMAR    useragent/core/UseragentGrammar.hpp
    NAMESPACE  useragent::Useragent
    EXTENSIONS .useragent
    OUT        useragent/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/useragent/core/*.cpp)
add_library(useragentCore ${_core})
target_link_libraries(useragentCore PUBLIC useragentGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/useragent/cli/*.cpp)
add_executable(useragent_cli ${_cli})
target_link_libraries(useragent_cli
                      PRIVATE useragentCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/useragent/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/useragent/lsp/main.cpp)
add_library(useragentLsp ${_lsp})
target_link_libraries(useragentLsp PUBLIC useragentCore cursiv::lsp)
add_executable(useragent_lsp useragent/lsp/main.cpp)
target_link_libraries(useragent_lsp PRIVATE useragentLsp)

cursiv_add_benchmark(useragent SOURCES useragent/bench/main.cpp)

cursiv_package_language(useragent)
