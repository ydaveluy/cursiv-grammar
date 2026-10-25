# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `tinyosnescGenerated` (the compiled library) and `tinyosnesc_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID tinyosnesc
    GRAMMAR    tinyosnesc/core/TinyosNescGrammar.hpp
    NAMESPACE  tinyosnesc::TinyosNesc
    EXTENSIONS .nc
    OUT        tinyosnesc/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tinyosnesc/core/*.cpp)
add_library(tinyosnescCore ${_core})
target_link_libraries(tinyosnescCore PUBLIC tinyosnescGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tinyosnesc/cli/*.cpp)
add_executable(tinyosnesc_cli ${_cli})
target_link_libraries(tinyosnesc_cli
                      PRIVATE tinyosnescCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tinyosnesc/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/tinyosnesc/lsp/main.cpp)
add_library(tinyosnescLsp ${_lsp})
target_link_libraries(tinyosnescLsp PUBLIC tinyosnescCore cursiv::lsp)
add_executable(tinyosnesc_lsp tinyosnesc/lsp/main.cpp)
target_link_libraries(tinyosnesc_lsp PRIVATE tinyosnescLsp)

cursiv_add_benchmark(tinyosnesc SOURCES tinyosnesc/bench/main.cpp)

cursiv_package_language(tinyosnesc)
