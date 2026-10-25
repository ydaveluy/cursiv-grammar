# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `bencodingGenerated` (the compiled library) and `bencoding_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID bencoding
    GRAMMAR    bencoding/core/BencodingGrammar.hpp
    NAMESPACE  bencoding::Bencoding
    EXTENSIONS .torrent
    OUT        bencoding/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bencoding/core/*.cpp)
add_library(bencodingCore ${_core})
target_link_libraries(bencodingCore PUBLIC bencodingGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bencoding/cli/*.cpp)
add_executable(bencoding_cli ${_cli})
target_link_libraries(bencoding_cli
                      PRIVATE bencodingCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bencoding/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/bencoding/lsp/main.cpp)
add_library(bencodingLsp ${_lsp})
target_link_libraries(bencodingLsp PUBLIC bencodingCore cursiv::lsp)
add_executable(bencoding_lsp bencoding/lsp/main.cpp)
target_link_libraries(bencoding_lsp PRIVATE bencodingLsp)

cursiv_add_benchmark(bencoding SOURCES bencoding/bench/main.cpp)

cursiv_package_language(bencoding)
