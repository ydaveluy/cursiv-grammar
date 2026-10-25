# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `mckeemanformGenerated` (the compiled library) and `mckeemanform_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID mckeemanform
    GRAMMAR    mckeemanform/core/MckeemanFormGrammar.hpp
    NAMESPACE  mckeemanform::MckeemanForm
    EXTENSIONS .mckeemanform
    OUT        mckeemanform/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/mckeemanform/core/*.cpp)
add_library(mckeemanformCore ${_core})
target_link_libraries(mckeemanformCore PUBLIC mckeemanformGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/mckeemanform/cli/*.cpp)
add_executable(mckeemanform_cli ${_cli})
target_link_libraries(mckeemanform_cli
                      PRIVATE mckeemanformCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/mckeemanform/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/mckeemanform/lsp/main.cpp)
add_library(mckeemanformLsp ${_lsp})
target_link_libraries(mckeemanformLsp PUBLIC mckeemanformCore cursiv::lsp)
add_executable(mckeemanform_lsp mckeemanform/lsp/main.cpp)
target_link_libraries(mckeemanform_lsp PRIVATE mckeemanformLsp)

cursiv_add_benchmark(mckeemanform SOURCES mckeemanform/bench/main.cpp)

cursiv_package_language(mckeemanform)
