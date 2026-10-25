# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `flowmaticGenerated` (the compiled library) and `flowmatic_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID flowmatic
    GRAMMAR    flowmatic/core/FlowmaticGrammar.hpp
    NAMESPACE  flowmatic::Flowmatic
    EXTENSIONS .flowmatic
    OUT        flowmatic/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/flowmatic/core/*.cpp)
add_library(flowmaticCore ${_core})
target_link_libraries(flowmaticCore PUBLIC flowmaticGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/flowmatic/cli/*.cpp)
add_executable(flowmatic_cli ${_cli})
target_link_libraries(flowmatic_cli
                      PRIVATE flowmaticCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/flowmatic/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/flowmatic/lsp/main.cpp)
add_library(flowmaticLsp ${_lsp})
target_link_libraries(flowmaticLsp PUBLIC flowmaticCore cursiv::lsp)
add_executable(flowmatic_lsp flowmatic/lsp/main.cpp)
target_link_libraries(flowmatic_lsp PRIVATE flowmaticLsp)

cursiv_add_benchmark(flowmatic SOURCES flowmatic/bench/main.cpp)

cursiv_package_language(flowmatic)
