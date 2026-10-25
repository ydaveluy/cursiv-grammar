# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `objcGenerated` (the compiled library) and `objc_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID objc
    GRAMMAR    objc/core/ObjcGrammar.hpp
    NAMESPACE  objc::Objc
    EXTENSIONS .m
    OUT        objc/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/objc/core/*.cpp)
add_library(objcCore ${_core})
target_link_libraries(objcCore PUBLIC objcGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/objc/cli/*.cpp)
add_executable(objc_cli ${_cli})
target_link_libraries(objc_cli
                      PRIVATE objcCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/objc/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/objc/lsp/main.cpp)
add_library(objcLsp ${_lsp})
target_link_libraries(objcLsp PUBLIC objcCore cursiv::lsp)
add_executable(objc_lsp objc/lsp/main.cpp)
target_link_libraries(objc_lsp PRIVATE objcLsp)

cursiv_add_benchmark(objc SOURCES objc/bench/main.cpp)

cursiv_package_language(objc)
