# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `smalltalkGenerated` (the compiled library) and `smalltalk_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID smalltalk
    GRAMMAR    smalltalk/core/SmalltalkGrammar.hpp
    NAMESPACE  smalltalk::Smalltalk
    EXTENSIONS .st
    OUT        smalltalk/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/smalltalk/core/*.cpp)
add_library(smalltalkCore ${_core})
target_link_libraries(smalltalkCore PUBLIC smalltalkGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/smalltalk/cli/*.cpp)
add_executable(smalltalk_cli ${_cli})
target_link_libraries(smalltalk_cli
                      PRIVATE smalltalkCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/smalltalk/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/smalltalk/lsp/main.cpp)
add_library(smalltalkLsp ${_lsp})
target_link_libraries(smalltalkLsp PUBLIC smalltalkCore cursiv::lsp)
add_executable(smalltalk_lsp smalltalk/lsp/main.cpp)
target_link_libraries(smalltalk_lsp PRIVATE smalltalkLsp)

cursiv_add_benchmark(smalltalk SOURCES smalltalk/bench/main.cpp)

cursiv_package_language(smalltalk)
