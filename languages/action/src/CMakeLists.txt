# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `actionGenerated` (the compiled library) and `action_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID action
    GRAMMAR    action/core/ActionGrammar.hpp
    NAMESPACE  action::Action
    EXTENSIONS .act
    OUT        action/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/action/core/*.cpp)
add_library(actionCore ${_core})
target_link_libraries(actionCore PUBLIC actionGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/action/cli/*.cpp)
add_executable(action_cli ${_cli})
target_link_libraries(action_cli
                      PRIVATE actionCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/action/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/action/lsp/main.cpp)
add_library(actionLsp ${_lsp})
target_link_libraries(actionLsp PUBLIC actionCore cursiv::lsp)
add_executable(action_lsp action/lsp/main.cpp)
target_link_libraries(action_lsp PRIVATE actionLsp)

cursiv_add_benchmark(action SOURCES action/bench/main.cpp)

cursiv_package_language(action)
